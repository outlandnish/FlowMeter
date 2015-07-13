/**
 * Flow Meter
 *
 * An Ardunino library that provides calibrated liquid flow and volume measurement with flow sensors.
 *
 * @author sekdiy (https://github.com/sekdiy/FlowMeter)
 * @date 14.07.2015
 * @version See git comments for changes.
 *
 */

#ifndef FLOWMETER_H
#define FLOWMETER_H

// Compatibility with the Arduino 1.0 library standard
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

typedef struct {
  double capacity;      //!< capacity, upper limit of flow rate (in l/min)
  double kFactor;       //!< "k-factor" (in (pulses/s) / (l/min)), e.g.: 1 pulse/s = kFactor * l/min
  double mFactor[10];   //!< "meter factor", multiplicative correction factor near unity (per decile of flow)
} FlowSensorProperties;

extern FlowSensorProperties FS400A;  //!< reference flow sensor properties

class FlowSensorCalibration {
  public:
    void setCapacity(double capacity) { this->_properties.capacity = capacity; }
    void setKFactor(double kFactor) { this->_properties.kFactor = kFactor; }
    void setMeterFactorPerDecile(unsigned int decile,
                                 unsigned int mFactor
                                ) { this->_properties.mFactor[decile] = mFactor; }

    FlowSensorProperties getProperties() { return this->_properties; }
    double getCapacity() { return this->_properties.capacity; }
    double getKFactor() { return this->_properties.kFactor; }
    unsigned int getMeterFactorPerDecile(unsigned int decile) { return this->_properties.mFactor[decile]; }

  protected:
    FlowSensorProperties _properties;
};

class FlowMeter
{
  public:
    FlowMeter(unsigned int pin = 2,             //!< The pin that the flow sensor is connected to (has to be interrupt capable, default INT0).
              FlowSensorProperties prop = FS400A //!< The properties of the actual flow sensor being used (default FS400A).
             );                                 //!< Constructor. Initializes a new flow meter object.

    unsigned int getPin();                      //!< A convenience method. @return Returns the Arduino pin number that the flow sensor is connected to.

    unsigned long getCurrentDuration();         //!< Returns the current tick duration (in s).
    double getCurrentFlowrate();                //!< Returns the current flow rate since last reset (in liters per tick duration).
    double getCurrentVolume();                  //!< Returns the current volume since last reset (in  liters per tick duration).
    double getCurrentError();                   //!< Returns the error resulting from the current measurement (as a probability between 0 and 1).

    unsigned long getTotalDuration();           //!< Returns the total run time of this flow meter instance (in s).
    double getTotalFlowrate();                  //!< Returns the averaged flow rate in this flow meter instance (in l/min).
    double getTotalVolume();                    //!< Returns the total volume flown trough this flow meter instance (in l).
    double getTotalError();                     //!< Returns the average error (as a probability between 0 and 1).

    /**
     * The tick method updates all internal calculations at the end of a measurement period.
     *
     * We're calculating flow and volume data over time.
     * The actual pulses have to be sampled using the count method (i.e. via an interrupt service routine).
     *
     * Flow sensor formulae:
     *
     * Let K: pulses per second per unit of measure (i.e. (1/s)/(l/min)),
     *     f: pulse frequency (1/s),
     *     Q: flow rate (l/min),
     *     p: sensor pulses (no dimension/unit),
     *     t: time since last measurements (s).
     *
     * K = f / Q             | units: (1/s) / (l/min) = (1/s) / (l/min)
     * <=>                   | Substitute p / t for f in order to allow for different measurement intervals
     * K = (p / t) / Q       | units: ((1/s)/(l/min)) = (1/s) / (l/min)
     * <=>                   | Solve for Q:
     * Q = (p / t) / K       | untis: l/min = 1/s / (1/s / (l/min))
     * <=>                   | Volume in l:
     * V = Q / 60            | units: l = (l/min) / (min)
     *
     * The property K is sometimes stated in pulses per liter or pulses per gallon.
     * In these cases the unit of measure has to be converted accordingly (e.g. from gal/s to l/min).
     * See file G34_Flow_rate_to_frequency.jpg for a reference.
     *
     * @param duration The tick duration (in ms).
     */
    void tick(unsigned long duration = 1000);
    void count();                              //!< Increments the internal pulse counter. Serves as an interrupt callback routine.
    void reset();                              //!< Prepares the flow meter for a fresh measurement. Resets all current values.

  protected:
    unsigned int _pin;                         //!< connection pin (has to be interrupt capable!)
    FlowSensorProperties _properties;           //!< sensor properties

    unsigned long _currentDuration;            //!< current tick duration (for normalisation purposes, in ms)
    double _currentFlowrate = 0.0f;            //!< current flow rate (in l/tick), e.g.: 1 l / min = 1 pulse / s / (pulses / s / l / min)
    double _currentVolume = 0.0f;              //!< current volume (in l), e.g.: 1 l = 1 (l / min) / (60 * s)
    double _currentCorrection;                 //!< currently applied correction factor

    unsigned long _totalDuration = 0.0f;       //!< total measured duration since begin of measurement (in ms)
    double _totalVolume = 0.0f;                //!< total volume since begin of measurement (in l)
    double _totalCorrection = 0.0f;            //!< accumulated correction factors

    volatile unsigned long _currentPulses = 0; //!< pulses within current sample period
};

#endif   // FLOWMETER_H