#include <arduinoFFT.h>
#include <SPI.h>

// Define FFT parameters
#define SAMPLES 128             // Must be a power of 2
#define SAMPLING_FREQUENCY 1000 // Hz, adjust as needed

double vReal[SAMPLES]; // Real part of the FFT
double vImag[SAMPLES]; // Imaginary part of the FFT

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

void fft()
{
  // Sample the signal
  for (int i = 0; i < SAMPLES; i++)
  {
    vReal[i] = analogRead(A0) * (5.0 / 1023.0); // Convert to voltage
    vImag[i] = 0.0;                             // Imaginary part is zero
    delay(1000 / SAMPLING_FREQUENCY);
  }

  // Perform FFT
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  // Analyze the FFT output
  double maxValue = 0;
  int maxIndex = -1;

  for (int i = 1; i < SAMPLES / 2; i++)
  {
    if (vReal[i] > maxValue)
    {
      maxValue = vReal[i];
      maxIndex = i;
    }
  }

  // Calculate frequency of the peak
  double frequency = (maxIndex * SAMPLING_FREQUENCY) / SAMPLES;

  Serial.print("Major peak @ ");
  Serial.println(FFT.majorPeak(),6);
  // Print results
  Serial.print("Peak frequency: ");
  Serial.print(frequency);
  Serial.println(" Hz");

  // Check if 60 Hz is detected
  if (abs(frequency - 60.0) < 1.0)
  {
    Serial.println("60 Hz signal detected!");
  }
  else
  {
    Serial.println("No 60 Hz signal detected.");
  }

  delay(500);
}
