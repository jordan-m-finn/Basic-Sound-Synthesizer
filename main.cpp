#include <iostream>
#include "olcNoiseMaker.h"

using namespace std;

// Coverts frequency (Hz) to angular velocity
double w(double dHertz)
{
	return dHertz * 2.0 * PI;
}

double osc(double dHertz, double dTime, int nType)
{
	switch (nType)
	{
	case 0: // sin wave
		return sin(w(dHertz) * dTime);

	case 1: // square wave
		return sin(w(dHertz) * dTime) > 0.0 ? 1.0 : -1.0;

	case 2: // triangle wave
		return (asin(sin(w(dHertz) * dTime)) * 2.0 / PI);\

	case 3: // saw wave (analogue, warm, slow)
	{
		double dOutput = 0.0;
		
		for (double n = 1.0; n < 50.0; n++)
			dOutput += (sin(n * w(dHertz) * dTime)) / n;

		return dOutput;
	}

	case 4: // saw wave (optimised, harsh, fast)
		return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

	case 5: // pseudo random noise
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
		
	default:
		return 0.0;
	}
}

struct sEnvelopeADSR
{
	double dAttackTime;
	double dDecayTime;
	double dReleaseTime;

	double dSustainAmplitude;
	double dStartAmplitude;

	double dTriggerOnTime;
	double dTriggerOffTime;

	bool bNoteOn;

	sEnvelopeADSR()
	{
		dAttackTime = 0.100;
		dDecayTime = 0.01;
		dStartAmplitude = 1.0;
		dSustainAmplitude = 0.8;
		dReleaseTime = 0.200;
		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
		bNoteOn = false;
	}

	double getAmplitude(double dTime)
	{
		double dAmplitude = 0.0;
		double dLifeTime = dTime - dTriggerOnTime;

		if (bNoteOn)
		{
			// ADS
			
			// Attack
			if (dLifeTime < dAttackTime)
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

			// Decay
			if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

			// Sustain
			if (dLifeTime > (dAttackTime + dDecayTime))
			{
				dAmplitude = dSustainAmplitude;
			}
		}
		else
		{
			// Release
			dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
		}

		if (dAmplitude <= 0.0001)
		{
			dAmplitude = 0;
		}

		return dAmplitude;
	}

	void NoteOn(double dTimeOn)
	{
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}

	void NoteOff(double dTimeOff)
	{
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}
};

atomic<double> dFrequencyOutput = 0.0;
sEnvelopeADSR envelope;
double dOctaveBaseFrequency = 60.0; // A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

double MakeNoise(double dTime)
{
	double dOutput = envelope.getAmplitude(dTime) *
		(
			+ osc(dFrequencyOutput * 0.5, dTime, 3)
			+ osc(dFrequencyOutput * 1.0, dTime, 1)
		);

	return dOutput * 0.4; // Master Volume
}

int main()
{
	wcout << "onelonecoder.com - Synthesizer Part 1" << endl;

	//Get all sound hardware
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	//Display findings
	for (auto d : devices) wcout << "Found Output Device:" << d << endl;

	//Create sound machine!
	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	//Link function with noise maker class
	sound.SetUserFunction(MakeNoise);

	// Add a keyboard like a piano
	int nCurrentKey = -1;
	bool bKeyPressed = false;
	while (1)
	{
		bKeyPressed = false;
		for (int k = 0; k < 16; k++)
		{	
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k])) & 0x8000)
			{
				if (nCurrentKey != k)
				{
					dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
					envelope.NoteOn(sound.GetTime());
					wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput << "Hz";
					nCurrentKey = k;
				}

				bKeyPressed = true;
			}
		}

		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				wcout << "\rNote Off: " << sound.GetTime() << "s                    ";
				envelope.NoteOff(sound.GetTime());
				nCurrentKey = -1;
			}
		}
	}

	return 0;
}