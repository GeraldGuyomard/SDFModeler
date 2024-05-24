//
//  PropertyCell.m
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 5/1/23.
//

#import "PropertyCell.h"

@implementation PropertyCell
{
    void* _object;
    Property::Ptr _property;
    ChangeCallback _changeCallback;
    
    BOOL _sliderHooked;
}

-(void*) object
{
    return _object;
}

-(Property::Ptr) property
{
    return _property;
}

- (IBAction)sliderValueChanged:(UISlider *)sender
{
    if (_changeCallback != nullptr)
    {
        _changeCallback(false, _object, _property, sender.value);
    }
}

- (IBAction)sliderValueEnded:(UISlider *)sender
{
    if (_changeCallback != nullptr)
    {
        _changeCallback(true    , _object, _property, sender.value);
    }
}

- (void)setupWithObject:(void*)object prop:(const Property::Ptr&)prop
{
    _object = object;
    _property = prop;
    
    self.namelabel.text = [NSString stringWithUTF8String:_property->name().c_str()];
    
    float minValue = 0.f;
    float maxValue = 0.f;
    float v = 0.f;
    
    const auto value = prop->get(_object);
    const auto& defaultValue = _property->defaultValue();
    if (const auto* range = std::get_if<FloatRangedValue>(&defaultValue))
    {
        minValue = range->minValue;
        maxValue = range->maxValue;
        v = std::get<float>(value);
    }
    else if (const auto* range = std::get_if<IntRangedValue>(&defaultValue))
    {
        minValue = range->minValue;
        maxValue = range->maxValue;
        v = std::get<int>(value);
    }
    
    self.valueSlider.minimumValue = minValue;
    self.valueSlider.maximumValue = maxValue;
    self.valueSlider.value = v;
    
    if (!_sliderHooked)
    {
        _sliderHooked = true;
        
        [self.valueSlider addTarget:self action:@selector(sliderValueChanged:) forControlEvents:UIControlEventValueChanged];
        [self.valueSlider addTarget:self action:@selector(sliderValueEnded:) forControlEvents:UIControlEventTouchUpInside];
    }
}

- (void)setChangeCallback:(const ChangeCallback &)cb
{
    _changeCallback = cb;
}

@end
