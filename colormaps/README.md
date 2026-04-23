# Colormaps

Each colormap is stored as a binary file containing 256 RGB triplets.  The triplets are stored as uint8 data in RGB order (RGB RGB RGB ...).

## Deployment

Use `scripts/embed_assets.py` to generate a header file that embeds the info in each binary file into source code.
