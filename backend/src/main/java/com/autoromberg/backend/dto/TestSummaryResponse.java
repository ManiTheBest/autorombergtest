package com.autoromberg.backend.dto;

import com.autoromberg.backend.domain.RombergTest;
import java.time.Instant;

public record TestSummaryResponse(
		Long id,
		Instant createdAt,
		Integer durationMs,
		Integer sampleCount) {

	public static TestSummaryResponse from(RombergTest test) {
		return new TestSummaryResponse(
				test.getId(),
				test.getCreatedAt(),
				test.getDurationMs(),
				test.getSampleCount());
	}
}
