#pragma once

#include <array>
#include <stddef.h>

inline static constexpr std::array<std::pair<std::array<float,2>,float>,1024> ggx_E
#include "GGX_corr_E.inl"
;

inline static constexpr std::array<std::array<float,2>,32> ggx_Eavg
#include "GGX_corr_Eavg.inl"
;

/* not needed for the current handling of diffuse roughness
inline static constexpr std::array<std::pair<std::array<float,2>,float>,1024> on_E
#include "OrenNayar_E.inl"
;

inline static constexpr std::array<std::array<float,2>,32> on_Eavg
#include "OrenNayar_Eavg.inl"
;
*/

template<size_t N>
inline float ms_lookup_Eavg(float key, const std::array<std::array<float,2>,N>& table)
{
  uint16_t l{0};
  uint16_t r = N;
  uint16_t m;

  while (l < r)
  {
    m = (l+r) / 2u;

    if (table[m][0] < key)
      l = m+1;
    else
      r = m;
  }

  return table[l-1][1];
}

template<size_t N>
inline float ms_lookup_E( const std::array<float,2>& key
                        , const std::array<std::pair<std::array<float,2>,float>,N>& table )
{
  uint16_t l{0};
  uint16_t r = N;
  uint16_t m;

  float alpha;
  while (l < r)
  {
    m = (l+r) / 2u;

    if (table[m].first[0] < key[0])
      l = m+1;
    else
      r = m;
  }
  alpha = table[l-1].first[0];

  l = (l < 31u) ? 0u : l-31u;
  r = (l+32u > N) ? N : l+32u;

  while (l < r)
  {
    m = (l+r) / 2u;

    if (table[m].first[0] > alpha)
    {
      r = m;
      continue;
    }

    if (table[m].first[0] < alpha)
    {
      l = m+1;
      continue;
    }

    if (table[m].first[1] < key[1])
      l = m+1;
    else
      r = m;
  }

  return table[l-1].second;
}