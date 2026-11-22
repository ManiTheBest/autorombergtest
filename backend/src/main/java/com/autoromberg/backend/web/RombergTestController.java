package com.autoromberg.backend.web;

import com.autoromberg.backend.dto.CreateTestRequest;
import com.autoromberg.backend.dto.TestDetailResponse;
import com.autoromberg.backend.dto.TestSummaryResponse;
import com.autoromberg.backend.service.RombergTestService;
import jakarta.validation.Valid;
import java.util.List;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/tests")
public class RombergTestController {

	private final RombergTestService service;

	public RombergTestController(RombergTestService service) {
		this.service = service;
	}

	@PostMapping
	public ResponseEntity<TestDetailResponse> create(@Valid @RequestBody CreateTestRequest request) {
		return ResponseEntity.status(201).body(service.create(request));
	}

	@GetMapping
	public List<TestSummaryResponse> list() {
		return service.list();
	}

	@GetMapping("/{id}")
	public TestDetailResponse get(@PathVariable Long id) {
		return service.get(id);
	}

	@GetMapping("/{id}/csv")
	public ResponseEntity<String> downloadCsv(@PathVariable Long id) {
		String csv = service.getCsv(id);
		return ResponseEntity.ok()
				.header(HttpHeaders.CONTENT_DISPOSITION, "attachment; filename=\"romberg-test-" + id + ".csv\"")
				.contentType(MediaType.parseMediaType("text/csv"))
				.body(csv);
	}
}
