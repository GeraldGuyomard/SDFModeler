//
//  Renderer.m
//  TestRayMarching
//
//  Created by Gérald Guyomard on 2/18/23.
//

#import "SelectionMattingRenderPass.h"


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


SelectionMattingRenderPass::SelectionMattingRenderPass(size_t cameraIndex)
: _inherited(cameraIndex), _cameraIndex(cameraIndex), _encodingDelegate(std::make_unique<MattingEncodingDelegate>())
{}

SelectionMattingRenderPass::~SelectionMattingRenderPass() = default;

bool
SelectionMattingRenderPass::init(Renderer& renderer)
{
    if(!_inherited::init(renderer))
    {
        return false;
    }
    
    _renderPassDescriptor = [MTLRenderPassDescriptor new];
    
    _renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    _renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    
    _renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
    _renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
    _renderPassDescriptor.depthAttachment.clearDepth = 0;
    
    return true;
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
    auto cameraRig = renderer.cameraRig();
    auto size = cameraRig->cameras()[_cameraIndex]->viewportSize();
    if ((size.x <= 0.f) || ((size.y <= 0.f)))
    {
        return nullptr;
    }
    
    // render at a lower resolution than final content
    // to save time and get free blur
    //size = ceil(size * 0.75f);
    
    if ((_targetDepthTexture.width != size.x) || (_targetDepthTexture.height != size.y))
    {
        auto config = pipelineConfiguration();
        
        const auto width = NSUInteger(size.x);
        const auto height = NSUInteger(size.y);
        
        auto device = renderer.mtlDevice();
        
        // Depth
        {
            const auto depthPixelFormat = renderer.delegate()->presentConfiguration()->depthPixelFormat;
            auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:depthPixelFormat width:width height:height mipmapped:NO];
            textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            
            _targetDepthTexture = [device newTextureWithDescriptor:textureDescriptor];
        }
    }
    
    _renderPassDescriptor.depthAttachment.texture = _targetDepthTexture;
    
    auto encoder = [cmdBuffer renderCommandEncoderWithDescriptor:_renderPassDescriptor];
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
