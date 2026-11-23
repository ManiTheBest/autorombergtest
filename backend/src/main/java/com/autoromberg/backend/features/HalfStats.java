package com.autoromberg.backend.features;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public record HalfStats(double median, double mean, double std, double skew, double kurtosis, boolean ok) {

	public static HalfStats of(List<Double> values) {
		if (values.size() < 2) {
			return new HalfStats(0, 0, 0, 0, 0, false);
		}
		List<Double> sorted = new ArrayList<>(values);
		Collections.sort(sorted);
		int n = sorted.size();
		double median = (n % 2 == 1)
				? sorted.get(n / 2)
				: 0.5 * (sorted.get(n / 2 - 1) + sorted.get(n / 2));
		double mean = 0;
		for (double value : values) {
			mean += value;
		}
		mean /= n;
		double variance = 0;
		for (double value : values) {
			double delta = value - mean;
			variance += delta * delta;
		}
		double std = Math.sqrt(variance / (n - 1));
		double skew = 0;
		double kurtosis = 0;
		if (std > 1e-12) {
			for (double value : values) {
				double z = (value - mean) / std;
				skew += z * z * z;
				kurtosis += z * z * z * z;
			}
			skew /= n;
			kurtosis /= n;
		}
		return new HalfStats(median, mean, std, skew, kurtosis, true);
	}

	public double[] minus(HalfStats other) {
		return new double[] {
				median - other.median,
				mean - other.mean,
				std - other.std,
				skew - other.skew,
				kurtosis - other.kurtosis
		};
	}
}
