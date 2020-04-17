//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef PXRUSDMAYAGL_HD_RENDERER_H
#define PXRUSDMAYAGL_HD_RENDERER_H

/// \file pxrUsdMayaGL/hdRenderer.h

#include <memory>
#include <vector>

// XXX: With Maya versions up through 2019 on Linux, M3dView.h ends up
// indirectly including an X11 header that #define's "Bool" as int:
//   - <maya/M3dView.h> includes <maya/MNativeWindowHdl.h>
//   - <maya/MNativeWindowHdl.h> includes <X11/Intrinsic.h>
//   - <X11/Intrinsic.h> includes <X11/Xlib.h>
//   - <X11/Xlib.h> does: "#define Bool int"
// This can cause compilation issues if <pxr/usd/sdf/types.h> is included
// afterwards, so to fix this, we ensure that it gets included first.
//
// The X11 include appears to have been removed in Maya 2020+, so this should
// no longer be an issue with later versions.
#include <pxr/usd/sdf/types.h>

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MSelectInfo.h>

#include <pxr/pxr.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

#include <mayaUsd/base/api.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Simple implementation of a Hydra renderer for a Maya shape.
///
/// This class is mainly intended as a "reference" implementation of how
/// an individual Maya shape type could be imaged by Hydra. The derived classes
/// of MPxSurfaceShapeUI (legacy viewport) and/or MPxDrawOverride (Viewport 2.0)
/// for the Maya shape would own an instance of this class and use it to
/// populate Hydra with scene data during Maya's draw prep phase, use Hydra to
/// draw in response to a draw callback, and handle selection requests in the
/// viewport.
///
/// Note that for production use, it is highly recommended that Maya shapes use
/// a derived class of PxrMayaHdShapeAdapter in combination with the
/// UsdMayaGLBatchRenderer instead. That combination should perform considerably
/// better than this renderer, since Hydra will be able to better take advantage
/// of batching larger numbers of shapes and preserving state between
/// draws/selections.
///
/// Typical usage of this class is as follows:
///
/// \code
/// getDrawRequests(...) {
///
///   ...
///
///   request.setToken(DRAW_SHADED_SMOOTH);
///   ...
///
/// }
/// \endcode
///
/// \code
/// draw(...) {
///
///   // gather data from the shape
///   ...
///
///   _hdRenderer.CheckRendererSetup(prim, excludePaths);
///
///   // create a params object and setup it up for the shape.
///   UsdImagingGLRenderParams params;
///   ...
///
///   // invoke the render
///   _hdRenderer.Render(..., params);
/// }
/// \endcode
class UsdMayaGLHdRenderer
{
public:

    /// \brief Enum for various drawing styles.  Should be used in \c
    /// getDrawRequests on the call to \c request.setToken.
    enum DRAWING_STYLES {
        DRAW_POINTS,
        DRAW_WIREFRAME,
        DRAW_SHADED_FLAT,
        DRAW_SHADED_SMOOTH,
        DRAW_BOUNDING_BOX
    };

    /// \brief struct to hold all the information needed for a
    /// viewport 2.0 draw request.
    struct RequestData {
        GfVec4f fWireframeColor;
        MBoundingBox bounds;
        MDrawRequest drawRequest;
    };
    typedef std::vector<RequestData> RequestDataArray;

    /// \brief Should be called when the prim to \p usdPrim to draw or \p
    /// excludePaths change
    MAYAUSD_CORE_PUBLIC
    void CheckRendererSetup(
            const UsdPrim& usdPrim,
            const SdfPathVector& excludePaths);

    /// \brief Generate an array of draw requests based on the selection status
    /// of \c objPath
    MAYAUSD_CORE_PUBLIC
    void GenerateDefaultVp2DrawRequests(
            const MDagPath& objPath,
            const MHWRender::MFrameContext& frameContext,
            const MBoundingBox& bounds,
            UsdMayaGLHdRenderer::RequestDataArray *requestArray);
    /// \brief Render the USD.
    ///
    /// This function overrides some of the members of \p params, in particular,
    /// the \c drawMode.
    MAYAUSD_CORE_PUBLIC
    void Render(
            const MDrawRequest& aRequest,
            M3dView& aView,
            UsdImagingGLRenderParams params) const;

    /// \brief Render the array of draw requests in viewport 2.0
    ///
    /// This function assumes that you have already set your desired values for
    /// \c complexity \c shotGuides and \c showRenderGuides members of
    /// \p params
    MAYAUSD_CORE_PUBLIC
    void RenderVp2(
        const RequestDataArray &requests,
        const MHWRender::MDrawContext& context,
        UsdImagingGLRenderParams params) const;

    /// \brief Test for intersection, for use in \c select().
    MAYAUSD_CORE_PUBLIC
    bool TestIntersection(
            MSelectInfo& selectInfo,
            UsdImagingGLRenderParams params,
            GfVec3d* hitPoint) const;

    /// \brief Helper function to convert from \p subdLevel (int) into Hydra's
    /// \p complexity parameter (\p float)
    MAYAUSD_CORE_PUBLIC
    static float SubdLevelToComplexity(int subdLevel);

private:
    UsdPrim _renderedPrim;
    SdfPathVector _excludePrimPaths;
    std::unique_ptr<UsdImagingGLEngine> _renderer;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
