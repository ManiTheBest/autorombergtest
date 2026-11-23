package com.autoromberg.backend.features;

import java.util.List;

public record LabeledRecording(int pairId, int eyesClosed, String sessionName) {

	public static List<LabeledRecording> catalog() {
		return List.of(
				new LabeledRecording(2, 0, "2025-05-09_15-40-13"),
				new LabeledRecording(2, 1, "2025-05-09_15-42-40"),
				new LabeledRecording(3, 0, "2025-05-09_16-08-59"),
				new LabeledRecording(3, 1, "2025-05-09_16-09-43"),
				new LabeledRecording(4, 0, "2025-05-09_16-57-02"),
				new LabeledRecording(4, 1, "2025-05-09_16-57-39"),
				new LabeledRecording(5, 0, "2025-05-09_18-00-12"),
				new LabeledRecording(5, 1, "2025-05-09_18-00-57"),
				new LabeledRecording(11, 0, "2025-05-10_20-41-36"),
				new LabeledRecording(11, 1, "2025-05-10_20-42-17"),
				new LabeledRecording(12, 0, "2025-05-10_21-15-51"),
				new LabeledRecording(12, 1, "2025-05-10_21-16-27"),
				new LabeledRecording(14, 1, "2025-05-10_23-47-08"),
				new LabeledRecording(14, 0, "2025-05-10_23-48-01"),
				new LabeledRecording(15, 1, "2025-05-10_23-49-01"),
				new LabeledRecording(15, 0, "2025-05-10_23-49-51"),
				new LabeledRecording(16, 1, "2025-05-10_23-53-26"),
				new LabeledRecording(16, 0, "2025-05-10_23-54-19"),
				new LabeledRecording(17, 1, "2025-05-11_23-23-19"),
				new LabeledRecording(17, 0, "2025-05-11_23-24-14"),
				new LabeledRecording(21, 1, "2025-05-12_00-00-08"),
				new LabeledRecording(21, 0, "2025-05-12_00-01-01"),
				new LabeledRecording(22, 1, "2025-05-12_00-02-13"),
				new LabeledRecording(22, 0, "2025-05-12_00-02-51"),
				new LabeledRecording(23, 1, "2025-05-12_16-14-36"),
				new LabeledRecording(23, 0, "2025-05-12_16-15-21"),
				new LabeledRecording(24, 1, "2025-05-12_16-16-49"),
				new LabeledRecording(24, 0, "2025-05-12_16-17-34"),
				new LabeledRecording(25, 1, "2025-05-12_17-42-53"),
				new LabeledRecording(25, 0, "2025-05-12_17-43-52"),
				new LabeledRecording(26, 1, "2025-05-12_01-23-27"),
				new LabeledRecording(26, 0, "2025-05-12_01-24-12"));
	}
}
