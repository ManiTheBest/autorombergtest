package com.autoromberg.backend.service;

import com.autoromberg.backend.domain.RombergTest;
import com.autoromberg.backend.dto.CreateTestRequest;
import com.autoromberg.backend.dto.TestDetailResponse;
import com.autoromberg.backend.dto.TestSummaryResponse;
import com.autoromberg.backend.repository.RombergTestRepository;
import java.util.List;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.server.ResponseStatusException;

@Service
public class RombergTestService {

	private final RombergTestRepository repository;

	public RombergTestService(RombergTestRepository repository) {
		this.repository = repository;
	}

	@Transactional
	public TestDetailResponse create(CreateTestRequest request) {
		RombergTest test = new RombergTest();
		test.setDurationMs(request.getDurationMs());
		test.setSampleCount(request.getSampleCount());
		test.setCsvContent(request.getCsvContent());
		return TestDetailResponse.from(repository.save(test));
	}

	@Transactional(readOnly = true)
	public List<TestSummaryResponse> list() {
		return repository.findAllByOrderByCreatedAtDesc().stream()
				.map(TestSummaryResponse::from)
				.toList();
	}

	@Transactional(readOnly = true)
	public TestDetailResponse get(Long id) {
		return TestDetailResponse.from(findOrThrow(id));
	}

	@Transactional(readOnly = true)
	public String getCsv(Long id) {
		return findOrThrow(id).getCsvContent();
	}

	private RombergTest findOrThrow(Long id) {
		return repository.findById(id)
				.orElseThrow(() -> new ResponseStatusException(HttpStatus.NOT_FOUND, "Test not found"));
	}
}
