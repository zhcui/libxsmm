int w_chunks = handle->ifwp/16;
int w_i, w;
int c_i;
element_input_type *base_addr;
const __m512i vgindex = _mm512_set_epi32(960,832,448,320,  704,576,192,64,  896,768,384,256,  640,512,128,0);
const __m256i shuffler = _mm256_set_epi32(7,5,3,1,6,4,2,0);

for (ifm1 = 0; ifm1 < handle->blocksifm_lp; ++ifm1) {
  for (ij = 0; ij < handle->ifhp; ++ij) {
    /* Handle full chunks  */
    for (w = 0; w < w_chunks; w++) {
      for (ifm2 = 0; ifm2 < 8; ++ifm2) {
        base_addr = &LIBXSMM_VLA_ACCESS(6, input_nopad, img, ifm1, ij, w*16, ifm2, 0, handle->blocksifm_lp, handle->ifhp, handle->ifwp, handle->ifmblock, handle->fm_lp_block);
        __m512i gather_reg = _mm512_i32gather_epi32(vgindex, base_addr, 1);
        __m256i lo_reg= _mm512_extracti64x4_epi64(gather_reg,0);
        __m256i hi_reg= _mm512_extracti64x4_epi64(gather_reg,1);
        __m256i compressed_low  = _mm256_unpacklo_epi16(lo_reg, hi_reg);
        compressed_low =  _mm256_permutevar8x32_epi32(compressed_low, shuffler);
        __m256i compressed_high  = _mm256_unpackhi_epi16(lo_reg, hi_reg);
        compressed_high =  _mm256_permutevar8x32_epi32(compressed_high, shuffler);
        __m256i compressed_low_store = _mm256_insertf128_si256(compressed_low_store, _mm256_extractf128_si256(compressed_low,0), 0);
        compressed_low_store = _mm256_insertf128_si256(compressed_low_store, _mm256_extractf128_si256(compressed_high, 0), 1);
        __m256i compressed_high_store = _mm256_insertf128_si256(compressed_high_store, _mm256_extractf128_si256(compressed_low,1), 0);
        compressed_high_store = _mm256_insertf128_si256(compressed_high_store, _mm256_extractf128_si256(compressed_high, 1), 1);
        _mm256_storeu_si256((union __m256i *) &LIBXSMM_VLA_ACCESS(5, tr_input_nopad, img, ifm1*2, ij, ifm2*2, w*16, BLOCKSIFM, handle->ifhp, handle->ifmblock, ifwp_extended), compressed_low_store);
        _mm256_storeu_si256((union __m256i *) &LIBXSMM_VLA_ACCESS(5, tr_input_nopad, img, ifm1*2, ij, ifm2*2+1, w*16, BLOCKSIFM, handle->ifhp, handle->ifmblock, ifwp_extended), compressed_high_store);
      }
      for (ifm2 = 8; ifm2 < handle->ifmblock; ++ifm2) {
        base_addr = &LIBXSMM_VLA_ACCESS(6, input_nopad, img, ifm1, ij, w*16, ifm2, 0, handle->blocksifm_lp, handle->ifhp, handle->ifwp, handle->ifmblock, handle->fm_lp_block);
        __m512i gather_reg = _mm512_i32gather_epi32(vgindex, base_addr, 1);
        __m256i lo_reg= _mm512_extracti64x4_epi64(gather_reg,0);
        __m256i hi_reg= _mm512_extracti64x4_epi64(gather_reg,1);
        __m256i compressed_low  = _mm256_unpacklo_epi16(lo_reg, hi_reg);
        compressed_low =  _mm256_permutevar8x32_epi32(compressed_low, shuffler);
        __m256i compressed_high  = _mm256_unpackhi_epi16(lo_reg, hi_reg);
        compressed_high =  _mm256_permutevar8x32_epi32(compressed_high, shuffler);
        __m256i compressed_low_store = _mm256_insertf128_si256(compressed_low_store, _mm256_extractf128_si256(compressed_low,0), 0);
        compressed_low_store = _mm256_insertf128_si256(compressed_low_store, _mm256_extractf128_si256(compressed_high, 0), 1);
        __m256i compressed_high_store = _mm256_insertf128_si256(compressed_high_store, _mm256_extractf128_si256(compressed_low,1), 0);
        compressed_high_store = _mm256_insertf128_si256(compressed_high_store, _mm256_extractf128_si256(compressed_high, 1), 1);
        _mm256_storeu_si256((union __m256i *) &LIBXSMM_VLA_ACCESS(5, tr_input_nopad, img, ifm1*2+1, ij, 2*ifm2-16, w*16, BLOCKSIFM, handle->ifhp, handle->ifmblock, ifwp_extended), compressed_low_store);
        _mm256_storeu_si256((union __m256i *) &LIBXSMM_VLA_ACCESS(5, tr_input_nopad, img, ifm1*2+1, ij, 2*ifm2-15, w*16, BLOCKSIFM, handle->ifhp, handle->ifmblock, ifwp_extended), compressed_high_store);
      }        
    }
  }
}