//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "Headers.h"

#include <cstring>
#include <assert.h>

template <typename TTransformedGeometry>
static void copy(
                 TransformedGeometryHeader* header,
                 const TTransformedGeometry& transformedGeometry)
{
    const auto geometryType = TTransformedGeometry::geometryType();
    assert(geometryType != GeometryType::invalid);
    
    const size_t size = sizeof(TTransformedGeometry);
    
    header->byteSize = uint32_t(alignedSize(size));
    header->transformedGeometryCode = computeTransformedGeometryCode(geometryType, TTransformedGeometry::transformerType());
    
    uint8_t* dst = &(header->firstByte);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(&transformedGeometry);
    
    memcpy(dst, src, size);
}

template <typename TTransformedGeometry>
INLINE void serializeTransformedGeometry(TransformedGeometryHeader* header, const TTransformedGeometry& transformedGeometry)
{
    copy(header, transformedGeometry);
}
