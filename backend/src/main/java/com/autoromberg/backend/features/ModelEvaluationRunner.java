package com.autoromberg.backend.features;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.CommandLineRunner;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.context.annotation.Profile;
import org.springframework.stereotype.Component;

@Component
@Profile("evaluate-models")
public class ModelEvaluationRunner implements CommandLineRunner {

	private static final Logger log = LoggerFactory.getLogger(ModelEvaluationRunner.class);

	private final ModelEvaluationService evaluationService;
	private final ConfigurableApplicationContext context;

	public ModelEvaluationRunner(ModelEvaluationService evaluationService, ConfigurableApplicationContext context) {
		this.evaluationService = evaluationService;
		this.context = context;
	}

	@Override
	public void run(String... args) throws Exception {
		var report = evaluationService.evaluate();
		log.info("Model report written: {}", report);
		int code = org.springframework.boot.SpringApplication.exit(context, () -> 0);
		System.exit(code);
	}
}
