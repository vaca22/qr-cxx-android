/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2021 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ConcentricFinder.h"
#include "DetectorResult.h"
#include "RegressionLine.h"
#include "PerspectiveTransform.h"
#include "QRVersion.h"

#include <vector>

namespace ZXing {

class DetectorResult;
class BitMatrix;

namespace QRCode {

	class QRDetector {
 	public:
		struct FinderPatternSet {
			ConcentricPattern bl, tl, tr;
		};
		struct DimensionEstimate
		{
			int dim = 0;
			double ms = 0;
			int err = 4;
		};

		using FinderPatterns = std::vector<ConcentricPattern>;
		using FinderPatternSets = std::vector<FinderPatternSet>;

		FinderPatterns FindFinderPatterns(const BitMatrix &image, bool tryHarder) const;

		FinderPatternSets GenerateFinderPatternSets(FinderPatterns &patterns) const;

		DetectorResult SampleQR(const BitMatrix &image, const FinderPatternSet &fp) const;

		DetectorResult SampleMQR(const BitMatrix &image, const ConcentricPattern &fp) const;

		DetectorResult DetectPureQR(const BitMatrix &image) const;

		DetectorResult DetectPureMQR(const BitMatrix &image) const;

		PatternView FindPattern(const PatternView &view) const;

		double EstimateModuleSize(const BitMatrix &image, ConcentricPattern a, ConcentricPattern b) const;




		ZXing::RegressionLine TraceLine(const BitMatrix &image, PointF p, PointF d, int edge) const;

		double EstimateTilt(const FinderPatternSet &fp) const;

		ZXing::PerspectiveTransform Mod2Pix(int dimension, PointF brOffset, QuadrilateralF pix) const;

		std::optional<PointF>
		LocateAlignmentPattern(const BitMatrix &image, int moduleSize, PointF estimate) const;

		const Version *
		ReadVersion(const BitMatrix &image, int dimension, const PerspectiveTransform &mod2Pix) const;

		DimensionEstimate
		EstimateDimension(const BitMatrix &image, ConcentricPattern a, ConcentricPattern b) const;
	};

}
// QRCode
} // ZXing
