# Introduction to Real-Time Computer Graphics Labs

- **Author**: Artur Pelcharskyi
- **OS**: Windows 11
- **API**: DirectX11

## Project
- **Presentation**: [Presentation](https://www.canva.com/design/DAG7RCk9kak/aRcvLGym1l9w0dpynTUWJg/edit?utm_content=DAG7RCk9kak&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton)

A simple CPU-based terrain generation system with on-demand mesh rebuilding. Terrain is generated only after pressing **Rebuild Terrain**, with a recommended maximum mesh size of **2048×2048** and **max height = 5**. Height values are produced using **Perlin noise** with adjustable **octaves, persistence**, and **lacunarity**, allowing control over overall detail and frequency.

The generator supports several **height curve multipliers**; the recommended option is the **Power** curve, where increasing the exponent produces sharper, steeper mountain shapes. Surface normals **are computed via cross products**, ensuring correct lighting. Shadow mapping is not supported, since the project is based on code from Lab Work #3.

Each normalized height value is mapped to a color gradient (deep water → coast → grasslands → rock → snow), making elevation visually distinguishable.
