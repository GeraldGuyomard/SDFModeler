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
                .padding(.top, 50)
            
            Button("Add Sphere") {
                VisionOSRenderer.addPrimitive(withName: "Sphere")
            }
            
            Button("Add Box") {
                VisionOSRenderer.addPrimitive(withName: "Box")
            }
            
            Button("Add Rounded Box") {
                VisionOSRenderer.addPrimitive(withName: "Rounded Box")
            }
            
            Button("Add Cylinder") {
                VisionOSRenderer.addPrimitive(withName: "Cylinder")
            }
            
            Button("Add Torus") {
                VisionOSRenderer.addPrimitive(withName: "Torus")
            }
            
            Text("--------")
            
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
