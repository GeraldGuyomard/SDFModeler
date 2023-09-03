//
//  PropertyCell.h
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 5/1/23.
//

#import <UIKit/UIKit.h>
#include "Type.h"

NS_ASSUME_NONNULL_BEGIN

@interface PropertyCell : UICollectionViewCell

@property(nonatomic) IBOutlet UILabel* namelabel;

- (void)setup:(const Property*)prop;

@end

NS_ASSUME_NONNULL_END
