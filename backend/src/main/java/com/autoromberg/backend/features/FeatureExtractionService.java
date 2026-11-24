package com.autoromberg.backend.features;

import com.autoromberg.backend.domain.RombergTest;
import com.autoromberg.backend.repository.RombergTestRepository;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

@Service
public class FeatureExtractionService {

	public static final double TILT_BLOCK = 0.02;

	private final RombergTestRepository repository;
	private final Path dataDir;

	public FeatureExtractionService(
			RombergTestRepository repository,
			@Value("${romberg.data-dir:../Data}") String dataDir) {
		this.repository = repository;
		this.dataDir = Path.of(dataDir);
	}

	public List<FeatureExample> extract() {
		repository.findBySource("sensor-logger");

		Map<Integer, List<Double>> openTilt = new HashMap<>();
		Map<Integer, List<Double>> closedTilt = new HashMap<>();
		for (LabeledRecording recording : LabeledRecording.catalog()) {
			String gravityCsv = readGravity(recording.sessionName());
			if (gravityCsv == null) {
				continue;
			}
			List<Double> tilt = tiltDegrees(parseZyx(gravityCsv));
			if (tilt.size() < 4) {
				continue;
			}
			if (recording.eyesClosed() == 0) {
				openTilt.put(recording.pairId(), tilt);
			} else {
				closedTilt.put(recording.pairId(), tilt);
			}
		}

		List<FeatureExample> examples = new ArrayList<>();
		for (int pairId : openTilt.keySet()) {
			List<Double> eo = openTilt.get(pairId);
			List<Double> ec = closedTilt.get(pairId);
			if (eo == null || ec == null) {
				continue;
			}
			int mid = eo.size() / 2;
			double[] eoFirst = tiltStats(eo.subList(0, mid));
			double[] eoSecond = tiltStats(eo.subList(mid, eo.size()));
			double[] eoFull = tiltStats(eo);
			double[] ecFull = tiltStats(ec);
			// Eyes-open control: second half vs first half of the open recording.
			examples.add(new FeatureExample(TILT_BLOCK, pairId, 0, minus(eoSecond, eoFirst)));
			// Eyes-closed: full closed recording vs full open recording.
			examples.add(new FeatureExample(TILT_BLOCK, pairId, 1, minus(ecFull, eoFull)));
		}
		return examples;
	}

	private String readGravity(String sessionName) {
		Path gravity = dataDir.resolve(sessionName).resolve("Gravity.csv");
		if (!Files.exists(gravity)) {
			return null;
		}
		try {
			return Files.readString(gravity);
		} catch (IOException ex) {
			return null;
		}
	}

	static double[] tiltStats(List<Double> angles) {
		int n = angles.size();
		double mean = 0;
		for (double angle : angles) {
			mean += angle;
		}
		mean /= n;
		double variance = 0;
		double square = 0;
		for (double angle : angles) {
			double delta = angle - mean;
			variance += delta * delta;
			square += angle * angle;
		}
		double std = Math.sqrt(variance / (n - 1));
		double rms = Math.sqrt(square / n);
		return new double[] {mean, std, rms};
	}

	static double[] minus(double[] left, double[] right) {
		return new double[] {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
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

	static List<Double> tiltDegrees(List<double[]> samples) {
		double mx = 0;
		double my = 0;
		double mz = 0;
		for (double[] sample : samples) {
			mx += sample[0];
			my += sample[1];
			mz += sample[2];
		}
		int n = samples.size();
		mx /= n;
		my /= n;
		mz /= n;
		double norm = Math.sqrt(mx * mx + my * my + mz * mz);
		if (norm < 1e-9) {
			norm = 1;
		}
		mx /= norm;
		my /= norm;
		mz /= norm;
		List<Double> angles = new ArrayList<>();
		for (double[] sample : samples) {
			double sampleNorm = Math.sqrt(sample[0] * sample[0] + sample[1] * sample[1] + sample[2] * sample[2]);
			if (sampleNorm < 1e-9) {
				sampleNorm = 1;
			}
			double dot = (sample[0] * mx + sample[1] * my + sample[2] * mz) / sampleNorm;
			dot = Math.max(-1.0, Math.min(1.0, dot));
			angles.add(Math.toDegrees(Math.acos(dot)));
		}
		return angles;
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
