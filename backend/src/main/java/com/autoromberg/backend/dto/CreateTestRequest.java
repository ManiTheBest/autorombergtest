package com.autoromberg.backend.dto;

import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public class CreateTestRequest {

	@NotNull
	@Min(1)
	private Integer durationMs;

	@NotNull
	@Min(1)
	private Integer sampleCount;

	@NotBlank
	private String csvContent;

	public Integer getDurationMs() {
		return durationMs;
	}

	public void setDurationMs(Integer durationMs) {
		this.durationMs = durationMs;
	}

	public Integer getSampleCount() {
		return sampleCount;
	}

	public void setSampleCount(Integer sampleCount) {
		this.sampleCount = sampleCount;
	}

	public String getCsvContent() {
		return csvContent;
	}

	public void setCsvContent(String csvContent) {
		this.csvContent = csvContent;
	}
}
