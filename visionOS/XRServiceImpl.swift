//
//  Renderer.swift
//

import CompositorServices
import Metal
import MetalKit
import simd
import Spatial

@objc class XRDrawableImpl : NSObject
{
    let _drawable: LayerRenderer.Drawable
    
    init(drawable: LayerRenderer.Drawable)
    {
        _drawable = drawable
    }
    
    @objc var depthRange: simd_float2
    {
        return _drawable.depthRange
    }
    
    @objc var viewCount : Int
    {
        return _drawable.views.count
    }
    
    @objc func localEyeTransform(_ index: Int) -> simd_float4x4
    {
        return _drawable.views[index].transform
    }
    
    @objc func tangents(_ index: Int) -> simd_float4
    {
        return _drawable.views[index].tangents
    }
    
    @objc func viewport(_ index: Int) -> MTLViewport
    {
        return _drawable.views[index].textureMap.viewport
    }
    
    @objc func colorTexture(_ index: Int) -> MTLTexture
    {
        return _drawable.colorTextures[index]
    }
    
    @objc func depthTexture(_ index: Int) -> MTLTexture
    {
        return _drawable.depthTextures[index]
    }
    
    @objc func present(_ cmdBuffer:MTLCommandBuffer)
    {
        _drawable.encodePresent(commandBuffer: cmdBuffer)
    }
}

@objc class XRFrameImpl : NSObject
{
    let _frame : LayerRenderer.Frame
    
    init(frame: LayerRenderer.Frame)
    {
        _frame = frame
    }
    
    @objc func startUpdate()
    {
        _frame.startUpdate();
    }
    
    @objc func endUpdate()
    {
        _frame.endUpdate();
    }
    
    @objc func startSubmission()
    {
        _frame.startSubmission()
    }
    
    @objc func endSubmission()
    {
        _frame.endSubmission()
    }
    
    @objc func waitUntilOptimalTime()-> Bool
    {
        guard let timing = _frame.predictTiming() else { return false }
        LayerRenderer.Clock().wait(until: timing.optimalInputTime)
        return true
    }
}

@objc class XRServiceImpl : NSObject
{
    override init()
    {
        _session = ARKitSession()
        _worldTrackingProvider = WorldTrackingProvider()
    }
    
    @objc public func start(completion: @escaping ()->Void)
    {
        Task {
            do {
                try await _session.run([_worldTrackingProvider])
            } catch {
                fatalError("Failed to initialize ARSession")
            }
            
            completion()
        }
    }
    
    @objc func queryNextFrame(layerRenderer:LayerRenderer) -> XRFrameImpl?
    {
        guard let frame = layerRenderer.queryNextFrame() else { return nil }
        return XRFrameImpl(frame: frame)
    }
    
    @objc func queryDrawable(frame: XRFrameImpl) -> XRDrawableImpl?
    {
        let f = frame._frame
        
        guard let d = f.queryDrawable() else {
            return nil
        }
        
        let drawable = XRDrawableImpl(drawable: d)
        
        let time = LayerRenderer.Clock.Instant.epoch.duration(to: d.frameTiming.presentationTime).timeInterval
        let deviceAnchor = _worldTrackingProvider.queryDeviceAnchor(atTimestamp: time)
        
        d.deviceAnchor = deviceAnchor
        
        return drawable
    }
    
    @objc func worldHeadTransform(_ drawable: XRDrawableImpl) -> simd_float4x4
    {
        guard let deviceAnchor = drawable._drawable.deviceAnchor else {
            return matrix_identity_float4x4
        }
        
        return deviceAnchor.originFromAnchorTransform
    }
    
    private let _session: ARKitSession
    private let _worldTrackingProvider: WorldTrackingProvider
}
