package com.autoromberg.backend.features;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

@Service
public class ModelEvaluationService {

	private final FeatureExtractionService features;
	private final Path binary;
	private final Path featureFile;
	private final Path metricsFile;
	private final Path uiMetricsFile;

	public ModelEvaluationService(
			FeatureExtractionService features,
			@Value("${rec.engine.binary:../rec_engine/romberg_cv}") String binary,
			@Value("${rec.engine.features:../rec_engine/output/features.txt}") String featureFile,
			@Value("${rec.engine.metrics:../rec_engine/output/model_metrics.json}") String metricsFile,
			@Value("${rec.engine.ui-metrics:../frontend/public/ml/model_metrics.json}") String uiMetricsFile) {
		this.features = features;
		this.binary = Path.of(binary).toAbsolutePath().normalize();
		this.featureFile = Path.of(featureFile).toAbsolutePath().normalize();
		this.metricsFile = Path.of(metricsFile).toAbsolutePath().normalize();
		this.uiMetricsFile = Path.of(uiMetricsFile).toAbsolutePath().normalize();
	}

	public String evaluate() throws Exception {
		List<FeatureExample> examples = features.extract();
		if (examples.size() < 16) {
			throw new IllegalStateException("Feature extraction produced " + examples.size()
					+ " examples. Expected labeled eyes-open and eyes-closed pairs in Neon.");
		}
		Files.createDirectories(featureFile.getParent());
		Files.createDirectories(metricsFile.getParent());
		StringBuilder body = new StringBuilder();
		body.append("# thresh pairId label med mean std skew kurtosis\n");
		for (FeatureExample example : examples) {
			body.append(example.toRow()).append('\n');
		}
		Files.writeString(featureFile, body.toString());

		Process process = new ProcessBuilder(
				binary.toString(),
				featureFile.toString(),
				metricsFile.toString())
				.redirectErrorStream(true)
				.start();
		String log = new String(process.getInputStream().readAllBytes());
		int code = process.waitFor();
		if (code != 0) {
			throw new IllegalStateException("C++ rec engine failed (" + code + "): " + log);
		}
		if (uiMetricsFile.getParent() != null) {
			Files.createDirectories(uiMetricsFile.getParent());
		}
		Files.copy(metricsFile, uiMetricsFile, java.nio.file.StandardCopyOption.REPLACE_EXISTING);
		return Files.readString(metricsFile);
	}
}
