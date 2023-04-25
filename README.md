# easyhive_kicad
KiCAD Design Files for easyhive PCB

1. Branch (master): Design with Nb-IoT Modul from ublox (SARA N310)
2. Branch: Design with Nb-IoT Modul from Quectel (BC66 04 Std)

Notes for Generate the bom of material:
1. copy file "easyhive_bom.py" to your local KiCad Plugin folder
e.g. /usr/share/kicad/plugins
2. Open Eeschema
3. Tools -> Generate Bill of Material: add a new plugin (choose easyhive_bom.py)
4. Generate BOM! Path is your KiCad project directory


### copyright
Software: GPL 3.0 and following versions   
Hardware: CERN OHL 1.2 and following versions   
   
This files are distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
