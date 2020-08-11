#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2020, Cyrille Favreau <cyrille.favreau@epfl.ch>
#
# This file is part of BioExplorer
# <https://github.com/BlueBrain/BioExplorer>
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License version 3.0 as published
# by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# All rights reserved. Do not distribute without further notice.

"""BioExplorer widgets"""

from brayns import CircuitExplorer
from ipywidgets import FloatSlider, Select, HBox, VBox, Layout, Button, SelectMultiple,\
    IntProgress, Checkbox, FloatText, IntText, ColorPicker, IntSlider, Label
import seaborn as sns
from IPython.display import display
from stringcase import pascalcase
import matplotlib

colormaps = [
    'Accent', 'Accent_r', 'Blues', 'Blues_r', 'BrBG', 'BrBG_r', 'BuGn', 'BuGn_r',
    'BuPu', 'BuPu_r', 'CMRmap', 'CMRmap_r', 'Dark2', 'Dark2_r', 'GnBu', 'GnBu_r',
    'Greens', 'Greens_r', 'Greys', 'Greys_r', 'OrRd', 'OrRd_r', 'Oranges', 'Oranges_r',
    'PRGn', 'PRGn_r', 'Paired', 'Paired_r', 'Pastel1', 'Pastel1_r', 'Pastel2', 'Pastel2_r',
    'PiYG', 'PiYG_r', 'PuBu', 'PuBuGn', 'PuBuGn_r', 'PuBu_r', 'PuOr', 'PuOr_r',
    'PuRd', 'PuRd_r', 'Purples', 'Purples_r', 'RdBu', 'RdBu_r', 'RdGy', 'RdGy_r',
    'RdPu', 'RdPu_r', 'RdYlBu', 'RdYlBu_r', 'RdYlGn', 'RdYlGn_r', 'Reds', 'Reds_r',
    'Set1', 'Set1_r', 'Set2', 'Set2_r', 'Set3', 'Set3_r',
    'Wistia', 'Wistia_r', 'YlGn', 'YlGnBu', 'YlGnBu_r', 'YlGn_r', 'YlOrBr',
    'YlOrBr_r', 'YlOrRd', 'YlOrRd_r', 'afmhot', 'afmhot_r', 'autumn', 'autumn_r',
    'binary', 'binary_r', 'bone', 'bone_r', 'brg', 'brg_r', 'bwr', 'bwr_r', 'cool', 'cool_r',
    'coolwarm', 'coolwarm_r', 'copper', 'copper_r', 'cubehelix', 'cubehelix_r', 'flag',
    'flag_r', 'gist_earth', 'gist_earth_r', 'gist_gray', 'gist_gray_r', 'gist_heat',
    'gist_heat_r', 'gist_ncar', 'gist_ncar_r', 'gist_rainbow', 'gist_rainbow_r', 'gist_stern',
    'gist_stern_r', 'gist_yarg', 'gist_yarg_r', 'gnuplot', 'gnuplot2', 'gnuplot2_r',
    'gnuplot_r', 'gray', 'gray_r', 'hot', 'hot_r', 'hsv', 'hsv_r', 'icefire', 'icefire_r',
    'inferno', 'inferno_r', 'jet_r', 'magma', 'magma_r', 'mako', 'mako_r',
    'nipy_spectral', 'nipy_spectral_r', 'ocean', 'ocean_r', 'pink', 'pink_r', 'plasma',
    'plasma_r', 'prism', 'prism_r', 'rainbow', 'rainbow_r', 'rocket', 'rocket_r', 'seismic',
    'seismic_r', 'spring', 'spring_r', 'summer', 'summer_r',
    'tab10', 'tab10_r', 'tab20', 'tab20_r', 'tab20b', 'tab20b_r', 'tab20c', 'tab20c_r',
    'terrain', 'terrain_r', 'viridis', 'viridis_r', 'vlag', 'vlag_r', 'winter', 'winter_r'
]

shading_modes = ['none', 'diffuse', 'electron', 'cartoon', 'electron_transparency', 'perlin', 'diffuse_transparency',
                 'checker']

default_grid_layout = Layout(border='1px solid black', margin='5px', padding='5px')
default_layout = Layout(width='50%', height='24px', display='flex', flex_flow='row')
style = {'description_width': 'initial', 'handle_color': 'gray'}


class Widgets:

    def __init__(self, client):
        """Create a new Circuit Explorer instance"""
        self._client = client.rockets_client
        self._brayns = client
        self._circuit_explorer = CircuitExplorer(client)

    def display_focal_distance(self):

        x_slider = FloatSlider(description='X', min=0, max=1, value=0.5)
        y_slider = FloatSlider(description='Y', min=0, max=1, value=0.5)
        a_slider = FloatSlider(description='Aperture', min=0, max=1, value=0)
        f_slider = FloatSlider(description='Focus radius', min=0, max=1, value=0.01)
        d_slider = FloatSlider(description='Focus distance', min=0, max=10000, value=0, disabled=True)
        f_button = Button(description='Refresh')

        class Updated:

            def __init__(self, brayns, circuit_explorer):
                self._brayns = brayns
                self._circuit_explorer = circuit_explorer
                self._widget_value = None
                self._x = 0.5
                self._y = 0.5
                self._aperture = 0.0
                self._focus_radius = 0.01
                self._focus_distance = 0.0
                self._nb_focus_points = 20

            def _update_camera(self):
                self._focus_distance = 0.0
                import random
                for n in range(self._nb_focus_points):
                    self._focus_distance = self._focus_distance + self._get_focal_distance(
                        (self._x + (random.random() - 0.5) * self._focus_radius,
                         self._y + (random.random() - 0.5) * self._focus_radius))

                self._focus_distance = self._focus_distance / self._nb_focus_points
                params = self._brayns.CircuitExplorerDofPerspectiveCameraParams()
                params.focus_distance = self._focus_distance
                params.aperture_radius = self._aperture
                params.enable_clipping_planes = True
                d_slider.value = self._focus_distance
                self._brayns.set_camera_params(params)

            def update(self):
                self._update_camera()

            def update_focus_radius(self, val_dict) -> None:
                self._widget_value = val_dict['new']
                self._focus_radius = self._widget_value
                self._update_camera()

            def update_aperture(self, val_dict) -> None:
                self._widget_value = val_dict['new']
                self._aperture = self._widget_value
                self._update_camera()

            def update_x(self, val_dict) -> None:
                self._widget_value = val_dict['new']
                self._x = self._widget_value
                self._update_camera()

            def update_y(self, val_dict) -> None:
                self._widget_value = val_dict['new']
                self._y = self._widget_value
                self._update_camera()

            def _get_focal_distance(self, coordinates=(0.5, 0.5)):
                """
                Return the focal distance for the specified normalized coordinates in the image

                :param list coordinates: Coordinates in the image
                :return: The focal distance
                :rtype: float
                """
                target = self._brayns.inspect(array=coordinates)['position']
                origin = self._brayns.camera.position.data
                v = [0, 0, 0]
                for k in range(3):
                    v[k] = float(target[k]) - float(origin[k])
                import math
                return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])

        update_class = Updated(self._brayns, self._circuit_explorer)

        def update_x(v):
            update_class.update_x(v)

        def update_y(v):
            update_class.update_y(v)

        def update_aperture(v):
            update_class.update_aperture(v)

        def update_focus_radius(v):
            update_class.update_focus_radius(v)

        def update_button(v):
            update_class.update()

        x_slider.observe(update_x, 'value')
        y_slider.observe(update_y, 'value')
        a_slider.observe(update_aperture, 'value')
        f_slider.observe(update_focus_radius, 'value')
        f_button.on_click(update_button)

        position_box = VBox([x_slider, y_slider, f_button])
        parameters_box = VBox([a_slider, f_slider, d_slider])
        horizontal_box = HBox([position_box, parameters_box], layout=default_grid_layout)
        display(horizontal_box)

    def display_palette_for_models(self):

        def set_colormap(model_id, colormap_name, shading_mode):
            material_ids = self._circuit_explorer.get_material_ids(model_id)['ids']
            nb_materials = len(material_ids)

            palette = sns.color_palette(colormap_name, nb_materials)
            self._circuit_explorer.set_material_extra_attributes(model_id=model_id)

            specular_exponents = list()
            shading_modes = list()
            colors = list()
            opacities = list()
            refraction_indices = list()
            reflection_indices = list()
            glossinesses = list()
            simulation_data_casts = list()
            user_parameters = list()
            emissions = list()

            for c in palette:
                colors.append((c[0], c[1], c[2]))
                shading_modes.append(shading_mode)
                specular_exponents.append(specular_exponent_slider.value)
                opacities.append(opacity_slider.value)
                refraction_indices.append(refraction_index_slider.value)
                reflection_indices.append(reflection_index_slider.value)
                glossinesses.append(glossiness_slider.value)
                simulation_data_casts.append(cast_simulation_checkbox.value)
                user_parameters.append(user_param_slider.value)
                emissions.append(emission_slider.value)

            self._circuit_explorer.set_materials(
                model_ids=[model_id], material_ids=material_ids,
                specular_exponents=specular_exponents, diffuse_colors=colors,
                specular_colors=colors, shading_modes=shading_modes, opacities=opacities,
                refraction_indices=refraction_indices, reflection_indices=reflection_indices,
                glossinesses=glossinesses, simulation_data_casts=simulation_data_casts,
                user_parameters=user_parameters, emissions=emissions
            )
            self._brayns.set_renderer(accumulation=True)

        ''' Models '''
        model_names = list()
        for model in self._brayns.scene.models:
            model_names.append(model['name'])
        model_combobox = Select(options=model_names, description='Models:', disabled=False)

        ''' Shading modes '''
        shading_combobox = Select(options=shading_modes, description='Shading:', disabled=False)

        ''' Colors '''
        palette_combobox = Select(options=colormaps, description='Palette:', disabled=False)

        ''' Events '''
        def update_materials_from_palette(v):
            set_colormap(self._brayns.scene.models[model_combobox.index]['id'], v['new'],
                         shading_combobox.index)

        def update_materials_from_shading(v):
            set_colormap(self._brayns.scene.models[model_combobox.index]['id'],
                         palette_combobox.value, shading_combobox.index)

        shading_combobox.observe(update_materials_from_shading, 'value')
        palette_combobox.observe(update_materials_from_palette, 'value')

        horizontal_box_list = HBox([model_combobox, shading_combobox, palette_combobox])

        opacity_slider = FloatSlider(description='Opacity', min=0, max=1, value=1)
        opacity_slider.observe(update_materials_from_shading)
        refraction_index_slider = FloatSlider(description='Refraction', min=1, max=5, value=1)
        refraction_index_slider.observe(update_materials_from_shading)
        reflection_index_slider = FloatSlider(description='Reflection', min=0, max=1, value=0)
        reflection_index_slider.observe(update_materials_from_shading)
        glossiness_slider = FloatSlider(description='Glossiness', min=0, max=1, value=1)
        glossiness_slider.observe(update_materials_from_shading)
        specular_exponent_slider = FloatSlider(description='Specular exponent', min=1, max=100,
                                               value=1)
        specular_exponent_slider.observe(update_materials_from_shading)
        user_param_slider = FloatSlider(description='User param', min=0, max=100, value=1)
        user_param_slider.observe(update_materials_from_shading)
        emission_slider = FloatSlider(description='Emission', min=0, max=100, value=0)
        emission_slider.observe(update_materials_from_shading)

        cast_simulation_checkbox = Checkbox(description='Simulation', value=False)
        cast_simulation_checkbox.observe(update_materials_from_shading)

        horizontal_box_detail1 = HBox([opacity_slider, refraction_index_slider,
                                       reflection_index_slider])
        horizontal_box_detail2 = HBox([glossiness_slider, specular_exponent_slider, user_param_slider])
        horizontal_box_detail3 = HBox([emission_slider, cast_simulation_checkbox])
        vertical_box = VBox([horizontal_box_list, horizontal_box_detail1, horizontal_box_detail2,
                             horizontal_box_detail3], layout=default_grid_layout)
        display(vertical_box)

    def display_model_visibility(self):
        model_names = list()
        for model in self._brayns.scene.models:
            model_names.append(model['name'])

        model_select = SelectMultiple(options=model_names, disabled=False)

        lbl_models=Label(value='Models:')
        show_btn = Button(description='Show')
        hide_btn = Button(description='Hide')
        lbl_aabb=Label(value='Bounds:')
        show_aabb_btn = Button(description='Show')
        hide_aabb_btn = Button(description='Hide')
        lbl_camera=Label(value='Camera:')
        adjust_camera_btn = Button(description='Adjust')
        vbox_visible = VBox([lbl_models, show_btn, hide_btn])
        vbox_aabb = VBox([lbl_aabb, show_aabb_btn, hide_aabb_btn])
        vbox_camera = VBox([lbl_camera, adjust_camera_btn])
        hbox_params = HBox([vbox_visible, vbox_aabb, vbox_camera])

        def update_models(visible):
            for model_id in model_select.index:
                self._brayns.update_model(
                    id=self._brayns.scene.models[model_id]['id'],
                    visible=visible)

        def show_models(event):
            update_models(True)

        def hide_models(event):
            update_models(False)

        def update_aabbs(visible):
            for model_id in model_select.index:
                self._brayns.update_model(
                    id=self._brayns.scene.models[model_id]['id'],
                    bounding_box=visible)

        def show_aabbs(event):
            update_aabbs(True)

        def hide_aabbs(event):
            update_aabbs(False)

        def adjust_camera(event):
            size_min_aabb = [1e6, 1e6, 1e6]
            size_max_aabb = [-1e6, -1e6, -1e6]
            for model_id in model_select.index:
                model = self._brayns.scene.models[model_id]
                bounds = model['bounds']
                min_aabb = bounds['min']
                max_aabb = bounds['max']
                half_size_aabb = [0, 0, 0]
                center = [0, 0, 0]
                for k in range(3):
                    half_size_aabb[k] = (max_aabb[k] - min_aabb[k]) / 2.0
                    center[k] = center[k] + (min_aabb[k] + max_aabb[k]) / 2.0

                for k in range(3):
                    size_min_aabb[k] = min(size_min_aabb[k], center[k] - half_size_aabb[k])
                    size_max_aabb[k] = max(size_max_aabb[k], center[k] + half_size_aabb[k])

            center_aabb = [0, 0, 0]
            diag = 0
            for k in range(3):
                diag = max(diag, size_max_aabb[k] - size_min_aabb[k])
                center_aabb[k] = (size_max_aabb[k] + size_min_aabb[k]) / 2.0

            origin = [0, 0, 0]
            for k in range(3):
                origin[k] = center_aabb[k]
                if k == 2:
                    origin[k] = origin[k] + diag * 1.5

            self._brayns.set_camera(position=origin, orientation=[0, 0, 0, 1], target=center_aabb)
            ''' Refresh UI '''
            self._brayns.set_renderer()

        show_btn.on_click(show_models)
        hide_btn.on_click(hide_models)
        show_aabb_btn.on_click(show_aabbs)
        hide_aabb_btn.on_click(hide_aabbs)
        adjust_camera_btn.on_click(adjust_camera)

        hbox = HBox([model_select, hbox_params], layout=default_grid_layout)
        display(hbox)

    def display_palette_for_transfer_function(self):

        def update_palette(brayns, model_id, palette_name):
            nb_points = max(2, min(256, nb_points_text.value))
            palette = sns.color_palette(palette_name, nb_points)

            btf = brayns.get_model_transfer_function(id=model_id)
            colors = list()
            points = list()

            step = 1.0 / float(nb_points - 1)
            for i in range(len(palette)):
                c = palette[i]
                colors.append((c[0], c[1], c[2]))
                points.append([i * step, alpha_slider.value])

            btf['colormap']['name'] = palette_name
            btf['colormap']['colors'] = colors
            btf['opacity_curve'] = points
            btf['range'] = [min_range.value, max_range.value]
            brayns.set_model_transfer_function(id=model_id, transfer_function=btf)

        ''' Models '''
        model_names = list()
        for model in self._brayns.scene.models:
            model_names.append(model['name'])
        model_combobox = Select(options=model_names, description='Models:', disabled=False)

        ''' Colors '''
        palette_combobox = Select(options=colormaps, description='Palette:', disabled=False)

        ''' Events '''
        def update_materials_from_palette(v):
            update_palette(self._brayns, self._brayns.scene.models[model_combobox.index]['id'],
                           v['new'])

        palette_combobox.observe(update_materials_from_palette, 'value')

        ''' Range '''
        min_range = FloatText(description='Min', value=-80)
        max_range = FloatText(description='Max', value=-10)
        nb_points_text = IntText(description='Points', value=16, min=2, max=256)
        alpha_slider = FloatSlider(description='Alpha', min=0, max=1, value=1)

        horizontal_box_list = HBox([model_combobox, palette_combobox])
        horizontal_box_range = HBox([min_range, max_range])
        horizontal_box_alpha = HBox([nb_points_text, alpha_slider])
        vertical_box = VBox([horizontal_box_list, horizontal_box_range, horizontal_box_alpha],
                            layout=default_grid_layout)
        display(vertical_box)

    def display_volume_parameters(self):

        def update_volume_parameters(v):
            self._brayns.set_volume_parameters(
                pre_integration=cb_pre_integration.value,
                specular=matplotlib.colors.to_rgb(cp_specular.value),
                gradient_shading=cb_gradient_shading.value,
                single_shade=cb_single_shade.value,
                adaptive_sampling=cb_adaptive_sampling.value,
                adaptive_max_sampling_rate=sl_max_sampling_rate.value,
                sampling_rate=sl_sampling_rate.value)

        cb_pre_integration = Checkbox(description='Preintegration', value=True)
        cb_pre_integration.observe(update_volume_parameters, 'value')
        cb_gradient_shading = Checkbox(description='Gradient shading', value=True)
        cb_gradient_shading.observe(update_volume_parameters, 'value')
        cb_single_shade = Checkbox(description='Single shade', value=True)
        cb_single_shade.observe(update_volume_parameters, 'value')
        cb_adaptive_sampling = Checkbox(description='Adaptive sampling', value=True)
        cb_adaptive_sampling.observe(update_volume_parameters, 'value')
        sl_sampling_rate = FloatSlider(description='Sampling', value=1, min=0.1, max=5)
        sl_sampling_rate.observe(update_volume_parameters, 'value')
        sl_max_sampling_rate = FloatSlider(description='Max sampling', value=1, min=0.1, max=5)
        sl_max_sampling_rate.observe(update_volume_parameters, 'value')
        cp_specular = ColorPicker(description='Specular', value='#FFFFFF')
        cp_specular.observe(update_volume_parameters, 'value')

        hb_sampling = HBox([sl_sampling_rate, sl_max_sampling_rate])
        hb_shading = HBox([cb_gradient_shading, cb_single_shade])
        hb_specular = HBox([cp_specular])
        hb_optimizations = HBox([cb_pre_integration, cb_adaptive_sampling])
        vb = VBox([hb_sampling, hb_optimizations, hb_shading, hb_specular],
                  layout=default_grid_layout)
        display(vb)

    def create_snapshot(self, size, output_folder, samples_per_pixel):

        application_params = self._brayns.get_application_parameters()
        renderer_params = self._brayns.get_renderer()
        old_image_stream_fps = application_params['image_stream_fps']
        old_viewport_size = application_params['viewport']
        old_samples_per_pixel = renderer_params['samples_per_pixel']
        old_max_accum_frames = renderer_params['max_accum_frames']
        self._brayns.set_renderer(samples_per_pixel=1, max_accum_frames=samples_per_pixel)
        self._brayns.set_application_parameters(viewport=size)
        self._brayns.set_application_parameters(image_stream_fps=0)

        af = list()
        cd = list()
        cam = self._circuit_explorer.get_camera()
        af.append(0)
        cd.append([cam['origin'], cam['direction'], cam['up'], cam['apertureRadius'],
                   cam['focusDistance']])

        self._circuit_explorer.export_frames_to_disk(
            start_frame=0,
            animation_frames=af,
            camera_definitions=cd,
            path=output_folder,
            samples_per_pixel=samples_per_pixel)

        progress = IntProgress(description='In progress...', min=0, max=100, value=0)
        display(progress)

        while self._circuit_explorer.get_export_frames_progress()['progress'] < 1.0:
            import time
            time.sleep(0.2)
            progress.value = \
                self._circuit_explorer.get_export_frames_progress()['progress'] * 100

        self._brayns.set_application_parameters(image_stream_fps=old_image_stream_fps,
                                                viewport=old_viewport_size)
        self._brayns.set_renderer(samples_per_pixel=old_samples_per_pixel,
                                  max_accum_frames=old_max_accum_frames)

        progress.description = 'Done'
        progress.value = 100

    def __display_advanced_settings(self, object_type):

        default_layout = Layout(width='50%', height='24px', display='flex', flex_flow='row')
        style = {'description_width': 'initial', 'handle_color': 'gray'}

        class_name = None
        if object_type == 'camera':
            class_name = pascalcase(self._brayns.get_camera()['current'])
            class_name += 'CameraParams'
        elif object_type == 'renderer':
            class_name = pascalcase(self._brayns.get_renderer()['current'])
            class_name += 'RendererParams'

        class_ = getattr(self._brayns, class_name)
        params = class_()

        widgets_list = dict()

        def update_params(v):
            for widget in widgets_list:
                params[widget] = widgets_list[widget].value
            if object_type == 'camera':
                self._brayns.set_camera_params(params)
            elif object_type == 'renderer':
                self._brayns.set_renderer_params(params)

        def get_value(props, key, default_value):
            try:
                return props[key]
            except KeyError as e:
                return default_value

        for param in params:
            p = params[param]
            props = p.__propinfo__['__literal__']
            t = props['type']
            value = params[param]
            name = props['raw_name']
            description = props['title']

            if t == 'number':
                minimum = get_value(props, 'minimum', 0)
                maximum = get_value(props, 'maximum', 1e4)
                f = FloatSlider(description=description, min=minimum, max=maximum, value=value,
                                style=style, layout=default_layout)
                f.observe(update_params, 'value')
                widgets_list[name] = f
            elif t == 'integer':
                minimum = get_value(props, 'minimum', 0)
                maximum = get_value(props, 'maximum', 1e4)
                f = IntSlider(description=description, min=minimum, max=maximum, value=value,
                              style=style, layout=default_layout)
                f.observe(update_params, 'value')
                widgets_list[name] = f
            elif t == 'boolean':
                b = Checkbox(description=description, value=False, style=style,
                             layout=default_layout)
                b.observe(update_params, 'value')
                widgets_list[name] = b
            elif t == 'string':
                b = Text(description=description, value=value, style=style, layout=default_layout)
                b.observe(update_params, 'value')
                widgets_list[name] = b

        vboxes = list()
        nb_widgets = len(widgets_list)
        nb_columns = 2
        for i in range(0, nb_widgets, nb_columns):
            box_widgets = list()

            for widget in list(widgets_list)[i:min(i + nb_columns, nb_widgets)]:
                box_widgets.append(widgets_list[widget])

            vboxes.append(HBox(box_widgets))

        hbox = VBox(vboxes, layout=default_grid_layout)
        display(hbox)

    def display_advanced_rendering_settings(self):
        self.__display_advanced_settings('renderer')

    def display_advanced_camera_settings(self):
        self.__display_advanced_settings('camera')

    def display_rendering_settings(self):

        def update_params(v):
            self._brayns.set_renderer(
                accumulation=checkbox_accumulation.value,
                background_color=matplotlib.colors.to_rgb(colorpicker_background.value),
                head_light=checkbox_head_light.value,
                samples_per_pixel=slider_samples_per_pixel.value,
                max_accum_frames=slider_max_accum_frames.value,
                subsampling=slider_sub_sampling.value
            )

        params = self._brayns.get_renderer()
        slider_samples_per_pixel = IntSlider(description='Samples per pixel', min=1, max=1024,
                                             value=params['samples_per_pixel'])
        slider_samples_per_pixel.observe(update_params)
        slider_max_accum_frames = IntSlider(description='Max accumulation frames', min=1, max=1024,
                                            value=params['max_accum_frames'])
        slider_max_accum_frames.observe(update_params, 'value')
        slider_sub_sampling = IntSlider(description='Sub-sampling', min=1, max=16,
                                        value=params['subsampling'])
        slider_sub_sampling.observe(update_params, 'value')
        colorpicker_background = ColorPicker(description='Background color',
                                             value=matplotlib.colors.to_hex(
                                                 params['background_color']))
        colorpicker_background.observe(update_params, 'value')
        checkbox_head_light = Checkbox(description='Head light', value=params['head_light'])
        checkbox_head_light.observe(update_params, 'value')
        checkbox_accumulation = Checkbox(description='Accumulation', value=params['accumulation'])
        checkbox_accumulation.observe(update_params, 'value')
        hbox_1 = HBox([slider_samples_per_pixel, slider_sub_sampling, slider_max_accum_frames])
        hbox_2 = HBox([colorpicker_background, checkbox_head_light, checkbox_accumulation])
        display(VBox([hbox_1, hbox_2], layout=default_grid_layout))

    def display_environment_maps(self, folder):
        import glob
        import os

        supported_extensions = ['jpg', 'jpeg', 'png']
        hdri_files = list()
        for extension in supported_extensions:
            hdri_files = hdri_files + glob.glob(folder + '/*.' + extension)
        hdri_files.sort()
        base_names = list()
        for hdri_file in hdri_files:
            base_names.append(os.path.basename(hdri_file))

        def update_envmap(v):
            filename = folder + '/' + v['new']
            self._brayns.set_environment_map(filename)

        cb_names = Select(description='Maps', options=base_names)
        cb_names.observe(update_envmap, 'value')
        display(cb_names)