package com.autoromberg.backend.features;

import com.autoromberg.backend.domain.RombergTest;
import com.autoromberg.backend.repository.RombergTestRepository;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.springframework.stereotype.Service;

@Service
public class FeatureExtractionService {

	public static final double FS = 100.0;

	private final RombergTestRepository repository;

	public FeatureExtractionService(RombergTestRepository repository) {
		this.repository = repository;
	}

	public List<FeatureExample> extract() {
		Map<String, String> csvBySession = new HashMap<>();
		for (RombergTest test : repository.findBySource("sensor-logger")) {
			if (test.getSessionName() != null && test.getCsvContent() != null) {
				csvBySession.put(test.getSessionName(), test.getCsvContent());
			}
		}

		record Series(int pairId, int eyesClosed, List<Double> pathLength) {
		}
		List<Series> series = new ArrayList<>();
		for (LabeledRecording recording : LabeledRecording.catalog()) {
			String csv = csvBySession.get(recording.sessionName());
			if (csv == null) {
				continue;
			}
			series.add(new Series(recording.pairId(), recording.eyesClosed(), pathLengths(parseZyx(csv), FS)));
		}

		List<FeatureExample> examples = new ArrayList<>();
		for (int step = 180; step <= 220; step += 5) {
			double thresh = step / 10000.0;
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

	static List<double[]> parseZyx(String csv) {
		String[] lines = csv.split("\\R");
		if (lines.length == 0) {
			return List.of();
		}
		String[] headers = lines[0].split(",");
		int iz = indexOf(headers, "z");
		int iy = indexOf(headers, "y");
		int ix = indexOf(headers, "x");
		List<double[]> rows = new ArrayList<>();
		for (int i = 1; i < lines.length; i++) {
			if (lines[i].isBlank()) {
				continue;
			}
			String[] cols = lines[i].split(",");
			int need = Math.max(iz, Math.max(iy, ix));
			if (cols.length <= need) {
				continue;
			}
			rows.add(new double[] {
					Double.parseDouble(cols[iz]),
					Double.parseDouble(cols[iy]),
					Double.parseDouble(cols[ix])
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
			double mx = 0, my = 0, mz = 0;
			for (int i = 0; i < nPer; i++) {
				mx += xx.get(start + i)[0];
				my += xx.get(start + i)[1];
				mz += xx.get(start + i)[2];
			}
			mx /= nPer;
			my /= nPer;
			mz /= nPer;
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
			if (headers[i].trim().equals(name)) {
				return i;
			}
		}
		throw new IllegalArgumentException("Missing column " + name);
	}
}
