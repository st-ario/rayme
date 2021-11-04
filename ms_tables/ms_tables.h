#pragma once

#include <array>

inline static constexpr std::array<std::pair<std::array<float,2>,float>,1024> ggx_E
#include "GGX_corr_E.inl"
;

inline static constexpr std::array<std::pair<std::array<float,2>,float>,1024> on_E
#include "OrenNayar_E.inl"
;

inline static constexpr std::array<std::array<float,2>,32> ggx_Eavg
#include "GGX_corr_Eavg.inl"
;

inline static constexpr std::array<std::array<float,2>,32> on_Eavg
#include "OrenNayar_Eavg.inl"
;

template<typename K, typename T>
inline float ms_lookup(K key, const T& table)
{
  uint16_t sel{0};
  uint16_t len{table.size()};
  uint16_t step{0};

  while (len != 0)
  {
    step = len / 2;

    if (std::get<0>(table[sel + step]) < key)
    {
      sel += ++step;
      len -= step;
    } else {
      len = step;
    }
  }

  return std::get<1>(table[sel]);
}

/*
float lookup(float alpha, const std::array<std::array<float,2>,32>& table);
float lookup( float cos_theta
            , float alpha
            , const std::array<std::pair<std::array<float,2>,float>,1024>& table);
*/