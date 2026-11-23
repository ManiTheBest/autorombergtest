package com.autoromberg.backend.importing;

import com.autoromberg.backend.domain.RombergTest;
import com.autoromberg.backend.repository.RombergTestRepository;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.stream.Stream;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.CommandLineRunner;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.context.annotation.Profile;
import org.springframework.stereotype.Component;

@Component
@Profile("import-phone")
public class PhoneDataImporter implements CommandLineRunner {

	private static final Logger log = LoggerFactory.getLogger(PhoneDataImporter.class);

	private final RombergTestRepository repository;
	private final ConfigurableApplicationContext context;
	private final Path dataDir;

	public PhoneDataImporter(
			RombergTestRepository repository,
			ConfigurableApplicationContext context,
			@Value("${phone.data-dir:../Data}") String dataDir) {
		this.repository = repository;
		this.context = context;
		this.dataDir = Path.of(dataDir).toAbsolutePath().normalize();
	}

	@Override
	public void run(String... args) throws Exception {
		if (!Files.isDirectory(dataDir)) {
			throw new IllegalStateException("Phone data directory not found: " + dataDir);
		}

		int saved = 0;
		int skipped = 0;
		try (Stream<Path> sessions = Files.list(dataDir)) {
			for (Path session : sessions.filter(Files::isDirectory).sorted(Comparator.comparing(Path::getFileName)).toList()) {
				Path csv = session.resolve("AccelerometerUncalibrated.csv");
				if (!Files.isRegularFile(csv)) {
					continue;
				}
				String sessionName = session.getFileName().toString();
				if (repository.existsBySessionName(sessionName)) {
					skipped++;
					log.info("Skip existing session {}", sessionName);
					continue;
				}
				String content = Files.readString(csv);
				RombergTest test = new RombergTest();
				test.setSessionName(sessionName);
				test.setSource("sensor-logger");
				test.setCsvContent(content);
				test.setSampleCount(countSamples(content));
				test.setDurationMs(durationMs(content));
				repository.save(test);
				saved++;
				log.info("Saved {} ({} samples)", sessionName, test.getSampleCount());
			}
		}

		log.info("Phone import done. saved={} skipped={}", saved, skipped);
		int code = org.springframework.boot.SpringApplication.exit(context, () -> 0);
		System.exit(code);
	}

	private static int countSamples(String csv) {
		int lines = 0;
		for (String line : csv.split("\\R")) {
			if (!line.isBlank()) {
				lines++;
			}
		}
		return Math.max(0, lines - 1);
	}

	private static int durationMs(String csv) {
		String last = null;
		for (String line : csv.split("\\R")) {
			if (!line.isBlank() && !line.startsWith("time,")) {
				last = line;
			}
		}
		if (last == null) {
			return 0;
		}
		String[] cols = last.split(",");
		if (cols.length < 2) {
			return 0;
		}
		double seconds = Double.parseDouble(cols[1]);
		return (int) Math.round(seconds * 1000.0);
	}
}
