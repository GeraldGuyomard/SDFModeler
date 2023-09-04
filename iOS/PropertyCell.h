//
//  PropertyCell.h
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 5/1/23.
//

#import <UIKit/UIKit.h>
#include "Type.h"

NS_ASSUME_NONNULL_BEGIN

using ChangeCallback = std::function<void(void* object, const Property* property, float newValue)>;

@interface PropertyCell : UICollectionViewCell

@property(nonatomic) IBOutlet UILabel* namelabel;
@property(nonatomic) IBOutlet UISlider* valueSlider;

@property(nonatomic) void* object;
@property(nonatomic) const Property* property;

- (void)setupWithObject:(void*)object prop:(const Property*)prop;

- (void)setChangeCallback:(const ChangeCallback&)cb;

@end

NS_ASSUME_NONNULL_END
