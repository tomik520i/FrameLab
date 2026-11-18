"""Resolution-aware native-pixel crops shared by diagnostic evidence exporters."""


def crop_boxes(width, height):
    if width <= 0 or height <= 0:
        raise ValueError('Invalid source dimensions')

    def scaled(box):
        x0, y0, x1, y1 = box
        result = (round(x0 * width / 2560), round(y0 * height / 1440),
                  round(x1 * width / 2560), round(y1 * height / 1440))
        if not (0 <= result[0] < result[2] <= width and 0 <= result[1] < result[3] <= height):
            raise ValueError('Crop lies outside the source image')
        return result

    return {'character': scaled((960, 400, 1600, 1040)),
            'hud': scaled((1920, 1040, 2560, 1440))}
