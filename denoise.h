#pragma once

class camera;
class image;

image denoise(image& noisy, image& albedo_map, image& normal_map);