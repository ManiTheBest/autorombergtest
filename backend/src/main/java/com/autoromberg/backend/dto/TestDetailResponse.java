package com.autoromberg.backend.dto;

import com.autoromberg.backend.domain.RombergTest;
import java.time.Instant;

public record TestDetailResponse(
		Long id,
		Instant createdAt,
		Integer durationMs,
		Integer sampleCount,
		String csvContent) {

	public static TestDetailResponse from(RombergTest test) {
		return new TestDetailResponse(
				test.getId(),
				test.getCreatedAt(),
				test.getDurationMs(),
				test.getSampleCount(),
				test.getCsvContent());
	}
}
