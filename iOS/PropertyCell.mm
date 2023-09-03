//
//  PropertyCell.m
//  SDFModeler iOS
//
//  Created by Gérald Guyomard on 5/1/23.
//

#import "PropertyCell.h"

@implementation PropertyCell

- (void)setup:(const Property*)prop
{
   self.namelabel.text = [NSString stringWithUTF8String:prop->name().c_str()];
}

@end
