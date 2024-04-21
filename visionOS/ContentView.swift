//
//  ContentView.swift
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 1/24/24.
//

import SwiftUI
import RealityKit
import RealityKitContent

struct ContentView: View {

    @State private var showImmersiveSpace = false
    @State private var immersiveSpaceIsShown = false

    @Environment(\.openImmersiveSpace) var openImmersiveSpace
    @Environment(\.dismissImmersiveSpace) var dismissImmersiveSpace

    var body: some View {
        VStack {
            Toggle("Show Immersive Space", isOn: $showImmersiveSpace)
                .toggleStyle(.button)
                .background() {
                    //Color(uiColor: UIColor.red)
                }
            
            Divider()
            
            VStack {
                let primitives = ["Sphere", "Box", "Rounded Box", "Cylinder", "Torus"]
                let n = primitives.count
                
                ForEach(0..<n) { i in
                    
                    let primitive = primitives[i]
                    let name = "Add \(primitive)"
                    
                    Button(name) {
                        VisionOSRenderer.addPrimitive(withName: primitive)
                    }
                }
                
            }.padding(EdgeInsets(top: 16.0, leading: 0.0, bottom: 16.0, trailing: 0.0))
            
            Divider()
            
            Button("Toggle Operation") {
                VisionOSRenderer.toggleOperation()
            }
        
        }
        .padding()
        .onChange(of: showImmersiveSpace) { _, newValue in
            Task {
                if newValue {
                    switch await openImmersiveSpace(id: "ImmersiveSpace") {
                    case .opened:
                        immersiveSpaceIsShown = true
                    case .error, .userCancelled:
                        fallthrough
                    @unknown default:
                        immersiveSpaceIsShown = false
                        showImmersiveSpace = false
                    }
                } else if immersiveSpaceIsShown {
                    await dismissImmersiveSpace()
                    immersiveSpaceIsShown = false
                }
            }
        }
    }
}

#Preview(windowStyle: .automatic) {
    ContentView()
}
