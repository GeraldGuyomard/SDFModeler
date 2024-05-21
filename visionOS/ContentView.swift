//
//  ContentView.swift
//  SDFModeler visionOS
//
//  Created by Gerald Guyomard on 1/24/24.
//

import SwiftUI

struct ContentView: View
{
    @Binding var appState: AppState
    @State private var showImmersiveSpace = false
    @State private var immersiveSpaceShown = false
    
    // these are functions given by the environment
    @Environment(\.openImmersiveSpace) var _openImmersiveSpace
    @Environment(\.dismissImmersiveSpace) var _dismissImmersiveSpace
    
    var body: some View
    {
        VStack {
            Toggle("Show Immersive Space", isOn: $showImmersiveSpace)
            .toggleStyle(.button)
            .background() {
                //Color(uiColor: UIColor.red)
            }
            
            if showImmersiveSpace
            {
                Divider()
                
                Section {
                    VStack {
                        let primitives = ["Sphere", "Box", "Rounded Box", "Cylinder", "Torus"]
                        
                        ForEach(primitives, id: \.self) { primitive in
                            
                            let name = "Add \(primitive)"
                            
                            Button(name) {
                                appState.renderer?.addPrimitive(withName: primitive)
                            }
                        }
                        
                    }.padding(EdgeInsets(top: 16.0, leading: 0.0, bottom: 16.0, trailing: 0.0))
                    
                    Divider()
                    
                    Button("Toggle Operation") {
                        appState.renderer?.toggleOperation()
                    }
                }
            }
        }
        .frame(width: 300, height: 600, alignment: Alignment.center)
        .onChange(of: showImmersiveSpace) { _, newValue in
            Task {
                if newValue
                {
                    self.immersiveSpaceShown = true
                    switch await _openImmersiveSpace(id: "ImmersiveSpace")
                    {
                        case .opened:
                            self.immersiveSpaceShown = true
                        case .error, .userCancelled:
                            fallthrough
                        @unknown default:
                            self.immersiveSpaceShown = false
                    }
                }
                else
                {
                    appState.tearDownImmersiveSpace()
                    await _dismissImmersiveSpace()

                    self.immersiveSpaceShown = false
                }
            }
        }
    }
}
