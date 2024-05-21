//
//  App.swift
//

import SwiftUI
import CompositorServices

struct ContentStageConfiguration: CompositorLayerConfiguration {
    func makeConfiguration(capabilities: LayerRenderer.Capabilities, configuration: inout LayerRenderer.Configuration) {
        configuration.depthFormat = .depth32Float
        configuration.colorFormat = .bgra8Unorm_srgb
    
        let foveationEnabled = capabilities.supportsFoveation
        configuration.isFoveationEnabled = foveationEnabled
        
        let options: LayerRenderer.Capabilities.SupportedLayoutsOptions = foveationEnabled ? [.foveationEnabled] : []
        let supportedLayouts = capabilities.supportedLayouts(options: options)
        
        configuration.layout = supportedLayouts.contains(.layered) ? .layered : .dedicated
    }
}

class AppState
{
    private var _renderer: VisionOSRenderer? = nil
    
    public var renderer: VisionOSRenderer?
    {
        return self._renderer
    }
    
    public func setupImmersiveSpace(layerRenderer: LayerRenderer)
    {
        self._renderer = VisionOSRenderer(layerRenderer: layerRenderer)
        self._renderer?.startRenderLoop()
    }
    
    public func tearDownImmersiveSpace()
    {
        if let renderer = self._renderer
        {
            renderer.shutdown();
        }
    }
}

@main
struct SpatialStudioApp: App
{
    @State private var appState = AppState()
    
    var body: some Scene
    {
        WindowGroup
        {
            ContentView(appState: $appState)
        }.windowResizability(.contentSize)

        ImmersiveSpace(id: "ImmersiveSpace")
        {
            CompositorLayer(configuration: ContentStageConfiguration())
            { layerRenderer in
                appState.setupImmersiveSpace(layerRenderer: layerRenderer)
            }
        }.immersionStyle(selection: .constant(.full), in: .full)
        
    }
}

