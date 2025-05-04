# bc5_dreamcast
BC5 codec for Dreamcast PowerVR

Compressed normal maps are decoded + twiddled simultaneously. This can be done directly into a `pvr_ptr_t` VRAM pointer.

Usage:

```
pvr_ptr_t pvr_texture_pointer = pvr_mem_malloc(width * height * 2);
uint8_t *compressed_normalmap_data = ...;
decode_bumpmap(compressed_normalmap_data, (uint8_t *)pvr_texture_pointer, width, height);
pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_BUMP | PVR_TXRFMT_TWIDDLED, width, height, pvr_texture_pointer, PVR_FILTER_BILINEAR);
```
