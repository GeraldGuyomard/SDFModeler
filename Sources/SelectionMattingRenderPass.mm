//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "SelectionMattingRenderPass.h"
#import <TargetConditionals.h>

class SelectionMattingRenderPass::MattingEncodingDelegate final : public EncodingContextDelegate
{
public:
    bool shouldEncode(const Object3D& object) const override
    {
        return _objectIDsToRender.find(object.id()) != _objectIDsToRender.end();
    }
    
    SDFOperation operation(const Object3D&) const override
    {
        return SDFOperation::addition;
    }
    
    bool hasObjectsToRender() const
    {
        return !_objectIDsToRender.empty();
    }
    
    void setObjectsToRender(const Object3DSelection& sel)
    {
        _objectIDsToRender.clear();
        
        for (const auto& object : sel.objects())
        {
            _addObjectToRender(object, false);
        }
    }
    
private:
    
    void _addObjectToRender(const Object3D::Ptr& object, bool ignoreIfSubstractive)
    {
        if (!ignoreIfSubstractive || (object->operation() == SDFOperation::addition))
        {
            if (object->geometry() != nullptr)
            {
                _objectIDsToRender.insert(object->id());
            }
            
            // also add parents
            auto parent = object->parent();
            while (parent != nullptr)
            {
                _objectIDsToRender.insert(parent->id());
                parent = parent->parent();
            }
        }
        
        for (const auto& child : object->children())
        {
            _addObjectToRender(child, true);
        }
    }
    
    std::unordered_set<ObjectID> _objectIDsToRender;
};


SelectionMattingRenderPass::SelectionMattingRenderPass()
: _encodingDelegate(std::make_unique<MattingEncodingDelegate>())
{}

SelectionMattingRenderPass::~SelectionMattingRenderPass() = default;

bool
SelectionMattingRenderPass::init(Renderer& renderer)
{
    if(!_inherited::init(renderer))
    {
        return false;
    }
    
    return true;
}

MTLRenderPassDescriptor* _Nullable
SelectionMattingRenderPass::makeRenderPassDescriptor(Renderer& renderer) const
{
    MTLRenderPassDescriptor* renderPassDescriptor = [renderer.delegate()->renderPassDescriptor(kLeftCameraIndex) copy];
    
    renderPassDescriptor.colorAttachments[0].texture = nil;
    
    renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
    renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
    renderPassDescriptor.depthAttachment.clearDepth = 0;
    
#if !TARGET_OS_SIMULATOR
    auto cameraRig = renderer.cameraRig();
    const auto cameras = cameraRig->cameras();
    
    renderPassDescriptor.renderTargetArrayLength = cameras.size();
#endif
    
    return renderPassDescriptor;
}

void
SelectionMattingRenderPass::updateUniforms(Renderer& renderer)
{
    enable(_encodingDelegate->hasObjectsToRender());
    
    if (enabled())
    {
        _inherited::updateUniforms(renderer);
    }
}

void
SelectionMattingRenderPass::configure(EncodingContext& ctx) const
{
    ctx.setDelegate(_encodingDelegate.get());
}

id<MTLRenderCommandEncoder>_Nullable
SelectionMattingRenderPass::makeRenderEncoder(Renderer& renderer, id<MTLCommandBuffer> _Nonnull cmdBuffer)
{
    const auto refRenderPassDescriptor = renderer.delegate()->renderPassDescriptor(kLeftCameraIndex);
    const auto depthTexture = refRenderPassDescriptor.depthAttachment.texture;
    const NSUInteger width = depthTexture.width;
    const NSUInteger height = depthTexture.height;
    
    if ((width == 0) || (height == 0))
    {
        return nullptr;
    }
    
    // render at a lower resolution than final content
    // to save time and get free blur
    //size = ceil(size * 0.75f);
    
    if ((_targetDepthTexture.width != width) || (_targetDepthTexture.height != height))
    {
        auto device = renderer.mtlDevice();
        
        // Depth
        {
            const auto depthPixelFormat = renderer.delegate()->presentConfiguration()->depthPixelFormat;
            auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:depthPixelFormat width:width height:height mipmapped:NO];
            
            textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            
        #if TARGET_OS_SIMULATOR
            textureDescriptor.storageMode = MTLStorageModePrivate;
        #else
            textureDescriptor.storageMode = MTLStorageModeShared;
        #endif
            
            
            textureDescriptor.textureType = MTLTextureType2DArray;
            textureDescriptor.arrayLength = depthTexture.arrayLength;
    
            _targetDepthTexture = [device newTextureWithDescriptor:textureDescriptor];
        }
    }
    
    auto renderPassDescriptor = makeRenderPassDescriptor(renderer);
    renderPassDescriptor.depthAttachment.texture = _targetDepthTexture;
    
    auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    encoder.label = @"SelectionMattingRenderPass";
    return encoder;
}

PipelineConfiguration::Ptr
SelectionMattingRenderPass::makePipelineConfiguration(Renderer& renderer) const
{
    auto config = _inherited::makePipelineConfiguration(renderer);
    
    config->pipelineName = "Selection Matting";
    
    auto mtlLib = renderer.mtlLibrary();
    config->fragmentFunction = [mtlLib newFunctionWithName:@"fragmentShaderMatting"];
    
    config->colorPixelFormat = MTLPixelFormatInvalid;
    
    return config;
}

void
SelectionMattingRenderPass::setObjectsToRender(const Object3DSelection& sel)
{
    _encodingDelegate->setObjectsToRender(sel);
}
