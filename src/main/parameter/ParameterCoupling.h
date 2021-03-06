#pragma once
#include <algorithm>
#include "../../types/GStructs.h"
#include "../../types/GTypes.h"

namespace guitard {
  /**
   * Struct/Class used to pair up a daw IParam if one is available and always a IControl + dsp parameter to be altered
   * Also contains the value bounds, step size, name and IParam index
   * A ParameterCoupling can also exist without a IParam which basically allows an unlimited
   * amount of IControls
   */
  class ParameterCoupling {
    sample mAdd = 0;
    sample mMul = 0;
    sample* value = nullptr; // pointer to the value used in the dsp code
    sample baseValue = 0; // This value is only used for params which couldn't claim a DAW parameter to act as the one provided by the IParam
#ifndef GUITARD_HEADLESS
    IParam*
#else
    void*
#endif
      parameter = nullptr; // Param object for outside daw automation
  public:
    enum Type {
      Auto,
      Linear,
      Boolean,
      Frequency,
      Gain,
      GainToLinear,
      Seconds,
      Milliseconds,
      Percentage
    };

    Type type = Auto;


    // Index of the IParam -1 means unassigned
    int parameterIdx = kNoParameter;

    // Bounds and so on 
    sample min = 0;
    sample max = 1;
    sample defaultVal = 0;
    sample stepSize = 0.01;

    // Control object which will draw the UI knob
#ifndef GUITARD_HEADLESS
    IControl* control = nullptr;
#endif
    // UI position and size
    Coord2D pos;
    Coord2D dim = { 90, 60 };
    float centerAngle = -135.f;
    float lowAngle = -135.f;
    float highAngle = 135.f;

    sample automation = 0; // this will be added on top of the actual value to allow internal automation eventually

    Node* automationDependency = nullptr;

    // name which may be used in the UI
    const char* name;

    bool showLabel = true;
    bool showValue = true;

    bool wantsDawParameter = true;

    explicit ParameterCoupling(const char* pName = nullptr, sample* pProperty = nullptr,
      const sample pDefault = 0.5, const sample pMin = 0, const sample pMax = 1,
      const sample pStepSize = 0.01, const Type pType = Auto)
    {
      value = pProperty;
      defaultVal = pDefault;
      baseValue = pDefault;
      min = pMin;
      max = pMax;
      if (std::abs(sample((max + min) * 0.5) - defaultVal) < sample(0.001)) {
        centerAngle = 0;
      }
      stepSize = pStepSize;
      name = pName;

      // Do some assumptions for the correct type
      if (pType == Auto) {
        guessType();
      }
      else {
        type = pType;
      }
    }

    void guessType() {
      if (max == 20000) {
        type = Frequency;
        if (min <= 0.) {
          min = 0.00000001;
        }
        mAdd = ::std::log(min);
        mMul = ::std::log(max / min);
      }
      else if (min == 0 && max == 1 && stepSize == 1) {
        type = Boolean;
      }
      else if (max >= 0 && min > -130 && min < -40) {
        type = Gain;
      }
      else if (min == 0 && max == 100) {
        type = Percentage;
      }
      else {
        type = Linear;
      }
    }

    /**
     * This should only be called from the audio thread since the value might tear on 32bit
     */
    void update() const {
      if (automationDependency == nullptr) {
        // No automation will just use the plain value
        *value = getValue();
      }
      else {
        // Will add the automation and clip the value
        *value = ::std::min(::std::max(getValue() + automation, min), max);
      }
    }

    /**
     * Simply returns the base Value. This is not the value used in the dsp code, since it never has automation applied
     */
    sample getValue() const {
#ifndef GUITARD_HEADLESS
      if (parameter != nullptr) {
        return static_cast<sample>(parameter->Value());
      }
#endif
      return baseValue;
    }

    void setValue(const sample v) {
#ifndef GUITARD_HEADLESS
      if (parameter != nullptr) {
        parameter->Set(v);
      }
      else
#endif
      {
        baseValue = v;
      }
    }

#ifndef GUITARD_HEADLESS
    void setParam(IParam* p) {
      parameter = p;
      if (p != nullptr) {
        switch (type) {
        case Boolean:
          parameter->InitBool(name, defaultVal > 0.5);
          break;
        case Frequency:
          parameter->InitFrequency(name, defaultVal, min, max, stepSize);
          break;
        case Gain:
          parameter->InitGain(name, defaultVal, min, max, stepSize);
          break;
        case Percentage:
          parameter->InitPercentage(name, defaultVal, min, max, stepSize);
          break;
        case Linear:
        default:
          parameter->InitDouble(name, defaultVal, min, max, stepSize);
          break;
        }
        p->Set(baseValue);
      }
    }
#endif

#ifndef GUITARD_HEADLESS
    IParam* getParam() {
      return parameter;
    }
#endif

    sample scaledToNormalized(const sample v) const {
      return ((v - min) / (max - min));
    }

    sample normalizedToScaled(const sample v) const {
      return min + v * (max - min);
    }


    sample normalizedToExp(const sample v) const {
      return ::std::exp(mAdd + v * mMul);
    }

    sample expToNormalized(const sample v) const {
      return (std::log(v) - mAdd) / mMul;
    }

    static sample dbToLinear(const sample v) {
      return pow(10, v / 20.0);
    }

    /**
     * Usually called from the IControl callback with a linear
     * normalized value, this will scale it to the internal DSP value
     */
    void setFromNormalized(const sample v) {
#ifndef GUITARD_HEADLESS
      if (parameter != nullptr) {
        parameter->SetNormalized(v);
      }
      else
#endif
      if (type == Frequency) {
        baseValue = normalizedToExp(v);
      }
      else {
        baseValue = normalizedToScaled(v);
      }
    }

    /**
     * Return the normalized and shaped value to use for controls
     */
    sample getNormalized() const {
#ifndef GUITARD_HEADLESS
      if (parameter != nullptr) {
        return parameter->GetNormalized();
      }
#endif
      /**
       * TODOG other types of scaling
       */
      if (type == Frequency) {
        return expToNormalized(baseValue);
      }
      return scaledToNormalized(baseValue);
    }

    /**
     * Sets the visual properties used when setting up a IControl based on this Coupling
     */
    void setPos(const float pX, const float pY, const float size = -1, const bool pShowLabel = true, const bool pShowValue = true) {
      pos.x = pX;
      pos.y = pY;
      if (size > 0) {
        dim.x = dim.y = size;
      }
      showLabel = pShowLabel;
      showValue = pShowValue;
    }

    template <typename T>
    T getValue() {
      return static_cast<T>(getValue());
    }

    template <typename T>
    T getWithAutomation() {
      return static_cast<T>(getValue() + automation);
    }
  };
}
