
.. _program_listing_file__home_favreau_git_BioExplorer_bioexplorer_core_module_ispc_camera_PerspectiveStereoCamera.h:

Program Listing for File PerspectiveStereoCamera.h
==================================================

|exhale_lsh| :ref:`Return to documentation for file <file__home_favreau_git_BioExplorer_bioexplorer_core_module_ispc_camera_PerspectiveStereoCamera.h>` (``/home/favreau/git/BioExplorer/bioexplorer/core/module/ispc/camera/PerspectiveStereoCamera.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   /*
    * The Blue Brain BioExplorer is a tool for scientists to extract and analyse
    * scientific data from visualization
    *
    * Copyright 2020-2021 Blue BrainProject / EPFL
    *
    * This program is free software: you can redistribute it and/or modify it under
    * the terms of the GNU General Public License as published by the Free Software
    * Foundation, either version 3 of the License, or (at your option) any later
    * version.
    *
    * This program is distributed in the hope that it will be useful, but WITHOUT
    * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    * details.
    *
    * You should have received a copy of the GNU General Public License along with
    * this program.  If not, see <https://www.gnu.org/licenses/>.
    */
   
   #pragma once
   
   #include "camera/Camera.h"
   
   #include <plugin/common/CommonTypes.h>
   
   namespace bioexplorer
   {
   namespace rendering
   {
   using namespace ospray;
   
   struct OSPRAY_SDK_INTERFACE PerspectiveStereoCamera : public Camera
   {
       PerspectiveStereoCamera();
   
       virtual std::string toString() const
       {
           return "bioexplorer::PerspectiveStereoCamera";
       }
   
       virtual void commit();
   
   public:
       float fovy;
       float aspect;
       float apertureRadius;
       float focusDistance;
       bool architectural;
   
       // Clip planes
       bool enableClippingPlanes{false};
       Ref<Data> clipPlanes;
   
       // Stereo
       CameraStereoMode stereoMode;
       float interpupillaryDistance;
   };
   } // namespace rendering
   } // namespace bioexplorer
