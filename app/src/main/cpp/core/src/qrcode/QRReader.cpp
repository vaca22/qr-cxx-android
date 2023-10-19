/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
* Copyright 2022 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include "QRReader.h"

#include "BinaryBitmap.h"
#include "ConcentricFinder.h"
#include "DecodeHints.h"
#include "DecoderResult.h"
#include "DetectorResult.h"
#include "LogMatrix.h"
#include "QRDecoder.h"
#include "QRDetector.h"
#include "Result.h"

#include <utility>

namespace ZXing::QRCode {

Result Reader::decode(const BinaryBitmap& image) const
{
	QRDetector qrDetector;
#if 1
	if (!_hints.isPure())
		return FirstOrDefault(decode(image, 1));
#endif

	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

	DetectorResult detectorResult;
	if (_hints.hasFormat(BarcodeFormat::QRCode))
		detectorResult = qrDetector.DetectPureQR(*binImg);
	if (_hints.hasFormat(BarcodeFormat::MicroQRCode) && !detectorResult.isValid())
		detectorResult =  qrDetector.DetectPureMQR(*binImg);

	if (!detectorResult.isValid())
		return {};

	auto decoderResult = Decode(detectorResult.bits());
	auto position = detectorResult.position();

	return Result(std::move(decoderResult), std::move(position),
				  detectorResult.bits().width() < 21 ? BarcodeFormat::MicroQRCode : BarcodeFormat::QRCode);
}

void logFPSet(const QRDetector::FinderPatternSet& fps [[maybe_unused]])
{
#ifdef PRINT_DEBUG
	auto drawLine = [](PointF a, PointF b) {
		int steps = maxAbsComponent(b - a);
		PointF dir = bresenhamDirection(PointF(b - a));
		for (int i = 0; i < steps; ++i)
			log(a + i * dir, 2);
	};

	drawLine(fps.bl, fps.tl);
	drawLine(fps.tl, fps.tr);
	drawLine(fps.tr, fps.bl);
#endif
}

Results Reader::decode(const BinaryBitmap& image, int maxSymbols) const
{
	QRDetector qrDetector;
	auto binImg = image.getBitMatrix();
	if (binImg == nullptr)
		return {};

#ifdef PRINT_DEBUG
	LogMatrixWriter lmw(log, *binImg, 5, "qr-log.pnm");
#endif

	auto allFPs = qrDetector.FindFinderPatterns(*binImg, _hints.tryHarder());

#ifdef PRINT_DEBUG
	printf("allFPs: %d\n", Size(allFPs));
#endif

	std::vector<ConcentricPattern> usedFPs;
	Results results;

	if (_hints.hasFormat(BarcodeFormat::QRCode)) {
		auto allFPSets =  qrDetector.GenerateFinderPatternSets(allFPs);
		for (const auto& fpSet : allFPSets) {
			if (Contains(usedFPs, fpSet.bl) || Contains(usedFPs, fpSet.tl) || Contains(usedFPs, fpSet.tr))
				continue;

			logFPSet(fpSet);

			auto detectorResult =  qrDetector.SampleQR(*binImg, fpSet);
			if (detectorResult.isValid()) {
				auto decoderResult = Decode(detectorResult.bits());
				auto position = detectorResult.position();
				if (decoderResult.isValid()) {
					usedFPs.push_back(fpSet.bl);
					usedFPs.push_back(fpSet.tl);
					usedFPs.push_back(fpSet.tr);
				}
				if (decoderResult.isValid(_hints.returnErrors())) {
					results.emplace_back(std::move(decoderResult), std::move(position), BarcodeFormat::QRCode);
					if (maxSymbols && Size(results) == maxSymbols)
						break;
				}
			}
		}
	}

	if (_hints.hasFormat(BarcodeFormat::MicroQRCode) && !(maxSymbols && Size(results) == maxSymbols)) {
		for (const auto& fp : allFPs) {
			if (Contains(usedFPs, fp))
				continue;

			auto detectorResult =  qrDetector.SampleMQR(*binImg, fp);
			if (detectorResult.isValid()) {
				auto decoderResult = Decode(detectorResult.bits());
				auto position = detectorResult.position();
				if (decoderResult.isValid(_hints.returnErrors())) {
					results.emplace_back(std::move(decoderResult), std::move(position), BarcodeFormat::MicroQRCode);
					if (maxSymbols && Size(results) == maxSymbols)
						break;
				}

			}
		}
	}

	return results;
}

} // namespace ZXing::QRCode
