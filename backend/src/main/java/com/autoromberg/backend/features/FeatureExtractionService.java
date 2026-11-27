package com.autoromberg.backend.features;

import com.autoromberg.backend.domain.RombergTest;
import com.autoromberg.backend.repository.RombergTestRepository;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.springframework.stereotype.Service;

@Service
public class FeatureExtractionService {

	/** ESP32 capture rate from romberg_capture.ino. */
	public static final double FS = 50.0;
	private static final Duration PAIR_WINDOW = Duration.ofMinutes(30);

	private final RombergTestRepository repository;

	public FeatureExtractionService(RombergTestRepository repository) {
		this.repository = repository;
	}

	public List<FeatureExample> extract() {
		List<DeviceSession> sessions = loadDeviceSessions();
		List<DevicePair> pairs = pairSessions(sessions);

		record Series(int pairId, int eyesClosed, List<Double> pathLength) {
		}
		List<Series> series = new ArrayList<>();
		for (DevicePair pair : pairs) {
			series.add(new Series(pair.pairId(), 0, pathLengths(parseAccel(pair.open().csvContent()), FS)));
			series.add(new Series(pair.pairId(), 1, pathLengths(parseAccel(pair.closed().csvContent()), FS)));
		}

		List<FeatureExample> examples = new ArrayList<>();
		// Device accel path lengths are larger than phone uncalibrated (~0.04–0.25).
		for (int step = 40; step <= 250; step += 10) {
			double thresh = step / 1000.0;
			Map<Integer, HalfStats> eo1 = new HashMap<>();
			Map<Integer, HalfStats> eo2 = new HashMap<>();
			Map<Integer, HalfStats> ec2 = new HashMap<>();
			for (Series item : series) {
				List<Double> kept = new ArrayList<>();
				for (double pl : item.pathLength()) {
					if (pl < thresh) {
						kept.add(pl);
					}
				}
				int mid = kept.size() / 2;
				HalfStats first = HalfStats.of(kept.subList(0, mid));
				HalfStats second = HalfStats.of(kept.subList(mid, kept.size()));
				if (item.eyesClosed() == 0) {
					eo1.put(item.pairId(), first);
					eo2.put(item.pairId(), second);
				} else {
					ec2.put(item.pairId(), second);
				}
			}
			for (int pairId : eo1.keySet()) {
				HalfStats openFirst = eo1.get(pairId);
				HalfStats openSecond = eo2.get(pairId);
				HalfStats closedSecond = ec2.get(pairId);
				if (openFirst == null || openSecond == null || closedSecond == null) {
					continue;
				}
				if (!openFirst.ok() || !openSecond.ok() || !closedSecond.ok()) {
					continue;
				}
				examples.add(new FeatureExample(thresh, pairId, 0, openSecond.minus(openFirst)));
				examples.add(new FeatureExample(thresh, pairId, 1, closedSecond.minus(openFirst)));
			}
		}
		return examples;
	}

	private List<DeviceSession> loadDeviceSessions() {
		List<DeviceSession> sessions = new ArrayList<>();
		for (RombergTest test : repository.findAllByOrderByCreatedAtDesc()) {
			if (test.getSource() != null && !test.getSource().isBlank()) {
				continue;
			}
			if (test.getSampleCount() == null || test.getSampleCount() < 1000) {
				continue;
			}
			if (test.getCsvContent() == null || test.getCsvContent().length() < 1000) {
				continue;
			}
			if (test.getEyesOpen() == null) {
				continue;
			}
			List<double[]> rows = parseAccel(test.getCsvContent());
			if (!hasMotion(rows)) {
				continue;
			}
			sessions.add(new DeviceSession(test.getId(), test.getCreatedAt(), test.getEyesOpen() == 1, test.getCsvContent()));
		}
		sessions.sort(Comparator.comparing(DeviceSession::createdAt));
		return sessions;
	}

	private static List<DevicePair> pairSessions(List<DeviceSession> sessions) {
		List<DevicePair> pairs = new ArrayList<>();
		Set<Long> used = new HashSet<>();
		int pairId = 1;
		for (int i = 0; i < sessions.size(); i++) {
			DeviceSession first = sessions.get(i);
			if (used.contains(first.id())) {
				continue;
			}
			for (int j = i + 1; j < sessions.size(); j++) {
				DeviceSession second = sessions.get(j);
				if (used.contains(second.id())) {
					continue;
				}
				if (first.eyesOpen() == second.eyesOpen()) {
					continue;
				}
				if (Duration.between(first.createdAt(), second.createdAt()).compareTo(PAIR_WINDOW) > 0) {
					break;
				}
				DeviceSession open = first.eyesOpen() ? first : second;
				DeviceSession closed = first.eyesOpen() ? second : first;
				pairs.add(new DevicePair(pairId++, open, closed));
				used.add(first.id());
				used.add(second.id());
				break;
			}
		}
		return pairs;
	}

	static boolean hasMotion(List<double[]> rows) {
		if (rows.size() < 10) {
			return false;
		}
		double mx = 0;
		double my = 0;
		double mz = 0;
		for (double[] row : rows) {
			mx += row[0];
			my += row[1];
			mz += row[2];
		}
		mx /= rows.size();
		my /= rows.size();
		mz /= rows.size();
		double variance = 0;
		for (double[] row : rows) {
			double dx = row[0] - mx;
			double dy = row[1] - my;
			double dz = row[2] - mz;
			variance += dx * dx + dy * dy + dz * dz;
		}
		variance /= rows.size();
		return variance > 1e-4;
	}

	static List<double[]> parseAccel(String csv) {
		String[] lines = csv.split("\\R");
		if (lines.length == 0) {
			return List.of();
		}
		String[] headers = lines[0].split(",");
		int ix;
		int iy;
		int iz;
		try {
			ix = indexOf(headers, "ax");
			iy = indexOf(headers, "ay");
			iz = indexOf(headers, "az");
		} catch (IllegalArgumentException missingAx) {
			ix = indexOf(headers, "x");
			iy = indexOf(headers, "y");
			iz = indexOf(headers, "z");
		}
		List<double[]> rows = new ArrayList<>();
		for (int i = 1; i < lines.length; i++) {
			if (lines[i].isBlank()) {
				continue;
			}
			String[] cols = lines[i].split(",");
			int need = Math.max(ix, Math.max(iy, iz));
			if (cols.length <= need) {
				continue;
			}
			rows.add(new double[] {
					Double.parseDouble(cols[ix]),
					Double.parseDouble(cols[iy]),
					Double.parseDouble(cols[iz])
			});
		}
		return rows;
	}

	static List<Double> pathLengths(List<double[]> xx, double fs) {
		int n = xx.size();
		int nPer = (int) Math.floor(3.0 * fs);
		int nInc = (int) Math.floor(fs);
		int nFrames = (int) Math.floor(n / (double) nInc) - 3 + 1;
		List<Double> pl = new ArrayList<>();
		int start = 0;
		for (int frame = 0; frame < nFrames; frame++) {
			if (start + nPer > n) {
				break;
			}
			double sum = 0;
			int count = 0;
			for (int i = 0; i < nPer - 1; i++) {
				double dx = xx.get(start + i + 1)[0] - xx.get(start + i)[0];
				double dy = xx.get(start + i + 1)[1] - xx.get(start + i)[1];
				double dz = xx.get(start + i + 1)[2] - xx.get(start + i)[2];
				sum += Math.sqrt(dx * dx + dy * dy + dz * dz);
				count++;
			}
			pl.add(count == 0 ? 0 : sum / count);
			start += nInc;
		}
		return pl;
	}

	private static int indexOf(String[] headers, String name) {
		for (int i = 0; i < headers.length; i++) {
			if (headers[i].trim().equalsIgnoreCase(name)) {
				return i;
			}
		}
		throw new IllegalArgumentException("Missing column " + name);
	}

	private record DeviceSession(long id, Instant createdAt, boolean eyesOpen, String csvContent) {
	}

	private record DevicePair(int pairId, DeviceSession open, DeviceSession closed) {
	}
}
