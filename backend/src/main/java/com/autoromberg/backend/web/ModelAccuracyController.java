package com.autoromberg.backend.web;

import com.autoromberg.backend.features.ModelEvaluationService;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/models")
public class ModelAccuracyController {

	private final ModelEvaluationService evaluationService;

	public ModelAccuracyController(ModelEvaluationService evaluationService) {
		this.evaluationService = evaluationService;
	}

	@GetMapping(value = "/accuracy", produces = MediaType.APPLICATION_JSON_VALUE)
	public ResponseEntity<String> accuracy() throws Exception {
		return ResponseEntity.ok(evaluationService.evaluate());
	}
}
