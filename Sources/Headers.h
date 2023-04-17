//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#pragma once

#include "CommonDefinitions.h"
#include "SDFGeometry/SDFGeometry.h"
#include "Transformer/Transformer.h"
#include "Material/Material.h"

CONSTANT static constexpr uint64_t kGeometryTypeShift = 2;

INLINE constexpr uint32_t computeTransformedGeometryCode(GeometryType geometryType, TransformerType transformerType)
{
    return (uint32_t(geometryType) << kGeometryTypeShift) | uint32_t(transformerType);
}

template <typename TGeometry, typename TTransformer>
INLINE constexpr uint64_t computeTransformedGeometryCode()
{
    return computeTransformedGeometryCode(TGeometry::type(), TTransformer::transformerType());
}

struct TransformedGeometryHeader final
{
    uint64_t  byteSize;
    uint64_t  transformedGeometryCode;
    
    uint8_t   firstByte; // TransformedGeometry<TGeometry, TTransformer>
    
    TransformedGeometryHeader(size_t byteSize, GeometryType geometryType, TransformerType transformerType)
    : byteSize(byteSize),
    transformedGeometryCode(computeTransformedGeometryCode(geometryType, transformerType))
    {}
    
    static CONSTANT TransformedGeometryHeader* next(CONSTANT TransformedGeometryHeader* header)
    {
        CONSTANT uint8_t* ptr = reinterpret_cast<CONSTANT uint8_t*>(header);
        return reinterpret_cast<CONSTANT TransformedGeometryHeader*>(ptr + header->byteSize);
    }
    
    static THREAD TransformedGeometryHeader* next(THREAD TransformedGeometryHeader* header)
    {
        THREAD uint8_t* ptr = reinterpret_cast<THREAD uint8_t*>(header);
        return reinterpret_cast<THREAD TransformedGeometryHeader*>(ptr + header->byteSize);
    }
};

template <typename TTransformedGeometry>
INLINE CONSTANT TTransformedGeometry* transformedGeometry(CONSTANT TransformedGeometryHeader* header)
{
    CONSTANT uint8_t* firstBytePtr = &(header->firstByte);
    return reinterpret_cast<CONSTANT TTransformedGeometry*>(firstBytePtr);
}

struct ObjectHeader
{
    uint32_t    objectID;
    uint32_t    materialID;
    bool        selected;
    
    ObjectHeader() = default;
    ObjectHeader(ObjectID objectID, MaterialID materialID, bool selected)
    : objectID(uint32_t(objectID)), materialID(uint32_t(materialID)), selected(selected)
    {}
};

struct SimpleObjectHeader final
{
    ObjectHeader objectHeader;
    uint64_t    geometryIndex;
    
    SimpleObjectHeader() = default;
    
    SimpleObjectHeader(ObjectHeader objectHeader, uint64_t geometryIndex)
    : objectHeader(objectHeader), geometryIndex(geometryIndex)
    {}
};

struct CompoundObjectHeader final
{
    uint64_t  byteSize;
    
    ObjectHeader objectHeader;
    
    uint32_t    nbPositiveGeometries;
    uint32_t    nbNegativeGeometries;
    
    CompoundObjectHeader() = default;
    
    CompoundObjectHeader(
                         ObjectHeader objectHeader,
                         size_t nbPositiveGeometries,
                         size_t nbNegativeGeometries)
    : objectHeader(objectHeader),
    nbPositiveGeometries(uint32_t(nbPositiveGeometries)),
    nbNegativeGeometries(uint32_t(nbNegativeGeometries))
    {}
    
    // after the list of positive geometry indices (uint32_t)
    // and the list of negative geometry indices (uint32_t)
    uint32_t firstPositiveIndex;
    
    static CONSTANT CompoundObjectHeader* next(CONSTANT CompoundObjectHeader* header)
    {
        CONSTANT uint8_t* ptr = reinterpret_cast<CONSTANT uint8_t*>(header);
        return reinterpret_cast<CONSTANT CompoundObjectHeader*>(ptr + header->byteSize);
    }
};
