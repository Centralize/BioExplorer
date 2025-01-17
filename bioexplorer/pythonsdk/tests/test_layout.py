#!/usr/bin/env python
"""Test protein layout"""

# -*- coding: utf-8 -*-

# The Blue Brain BioExplorer is a tool for scientists to extract and analyse
# scientific data from visualization
#
# Copyright 2020-2021 Blue BrainProject / EPFL
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <https://www.gnu.org/licenses/>.

from bioexplorer import BioExplorer, Protein, Surfactant, Vector3, Quaternion

# pylint: disable=no-member
# pylint: disable=missing-function-docstring


def test_layout():
    resource_folder = 'tests/test_files/'
    pdb_folder = resource_folder + 'pdb/'

    bio_explorer = BioExplorer('localhost:5000')
    bio_explorer.reset()
    print('BioExplorer version ' + bio_explorer.version())

    # Suspend image streaming
    bio_explorer.core_api().set_application_parameters(image_stream_fps=0)

    line_surfactant = 5
    line_virus = 25
    line_defense = 45

    # Camera
    brayns = bio_explorer.core_api()
    brayns.set_camera(
        current='orthographic',
        orientation=[0.0, 0.0, 0.0, 1.0],
        position=[23.927943790322814, 24.84577580212592, 260.43975983632527],
        target=[23.927943790322814, 24.84577580212592, 39.93749999999999]
    )
    params = brayns.OrthographicCameraParams()
    params.height = 55
    brayns.set_camera_params(params)

    # Grid
    bio_explorer.add_grid(min_value=0, max_value=100, interval=1, radius=0.005, colored=False,
                          position=Vector3(-10.0, -10.0, -10.0))

    # Layout
    virus_protein_s = Protein(
        sources=[
            pdb_folder + '6vyb.pdb',  # Open conformation
            pdb_folder + 'sars-cov-2-v1.pdb'  # Closed conformation
        ])

    bio_explorer.add_protein(name='Protein S (open)', protein=virus_protein_s,
                             conformation_index=0,
                             position=Vector3(5.0, line_virus, 0.0),
                             rotation=Quaternion(0.0, 0.0, 1.0, 0.0))
    bio_explorer.add_protein(name='Protein S (closed)', protein=virus_protein_s,
                             conformation_index=1,
                             position=Vector3(20.0, line_virus, 0.0),
                             rotation=Quaternion(0.0, 0.0, 1.0, 0.0))

    # Protein M (QHD43419)
    virus_protein_m = Protein(sources=[pdb_folder + 'QHD43419a.pdb'])
    bio_explorer.add_protein(name='Protein M', protein=virus_protein_m,
                             position=Vector3(35.0, line_virus, 0.0))

    # Protein E (QHD43418 P0DTC4)
    virus_protein_e = Protein(sources=[pdb_folder + 'QHD43418a.pdb'])
    bio_explorer.add_protein(name='Protein E', protein=virus_protein_e,
                             position=Vector3(45.0, line_virus, 0.0))

    # Lactoferrin
    lactoferrin = Protein(sources=[pdb_folder + 'immune/1b0l.pdb'])
    bio_explorer.add_protein(name='Lactoferrin', protein=lactoferrin,
                             position=Vector3(5.0, line_defense, 0.0))

    # Defensin
    defensin = Protein(sources=[pdb_folder + 'immune/1ijv.pdb'])
    bio_explorer.add_protein(name='Defensin', protein=defensin,
                             position=Vector3(20.0, line_defense, 0.0))

    # Glucose
    glucose = Protein(sources=[pdb_folder + 'glucose.pdb'], load_non_polymer_chemicals=True)
    bio_explorer.add_protein(name='Glucose', protein=glucose,
                             position=Vector3(30.0, line_defense, 0.0),
                             rotation=Quaternion(0.0, 0.0, 0.707, 0.707))

    # ACE2 Receptor
    ace2_receptor = Protein(sources=[pdb_folder + '6m18.pdb'])
    bio_explorer.add_protein(name='ACE2 receptor', protein=ace2_receptor,
                             position=Vector3(45.0, line_defense - 2.5, 0.0),
                             rotation=Quaternion(0.5, 0.5, 1.0, 0.0))

    # Surfactant
    head_source = pdb_folder + 'surfactant/1pw9.pdb'
    branch_source = pdb_folder + 'surfactant/1k6f.pdb'
    surfactant_d = Surfactant(
        name='Surfactant',
        surfactant_protein=bio_explorer.SURFACTANT_BRANCH,
        head_source=head_source,
        branch_source=branch_source
    )

    bio_explorer.add_surfactant(
        surfactant=surfactant_d, position=Vector3(5.0, line_surfactant, 0.0)
    )

    # Restore image streaming
    bio_explorer.core_api().set_application_parameters(image_stream_fps=20)


if __name__ == '__main__':
    import nose

    nose.run(defaultTest=__name__)
