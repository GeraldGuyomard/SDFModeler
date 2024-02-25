//
//  SDFModeler
//
//  Created by Gérald Guyomard on 2/26/23.
//

#include "Object3D.h"

// some static initializers
TObject3DFactoryRegistration s_SphereRegistration {"Sphere", SDFSphere { 0.5f } };
TObject3DFactoryRegistration s_BoxRegistration {"Box", SDFBox { float3 {0.5f, 0.5f, 0.5} } };
TObject3DFactoryRegistration s_RoundedBoxRegistration {"Rounded Box", SDFRoundedBox { float3 {0.5f, 0.5f, 0.5}, 0.1f } };
TObject3DFactoryRegistration s_TorusRegistration {"Torus", SDFTorus { 0.5f, 0.25f } };
TObject3DFactoryRegistration s_CylinderRegistration {"Cylinder", SDFCylinder { 0.3f, 0.7f } };
