#pragma once

class camera;
class image;

image denoise(const camera& cam, const image& noisy);