#include "SampleUtil.h"
#include "animatePorcess.h"
#include "global.h"

void WriteMotionBegin(const SampleTimeSet &sampleTimes)
{
	std::vector<float> outputTimes;
	outputTimes.reserve(sampleTimes.size());

	chrono_t frameTime = frame / 24.0;

	for (SampleTimeSet::const_iterator iter = sampleTimes.begin();
		iter != sampleTimes.end(); ++iter)
	{
		// why is this static?
		static const chrono_t epsilon = 1.0 / 10000.0;

		float value = ((*iter) - frameTime) * 24.0;

		if (fabs(value) < epsilon)
		{
			value = 0.0f;
		}

		outputTimes.push_back(value);
	}
}