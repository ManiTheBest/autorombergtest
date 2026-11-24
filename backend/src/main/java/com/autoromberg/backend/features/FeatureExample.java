package com.autoromberg.backend.features;

public record FeatureExample(double thresh, int pairId, int label, double[] features) {

	public String toRow() {
		return thresh + " " + pairId + " " + label + " "
				+ features[0] + " " + features[1] + " " + features[2];
	}
}
