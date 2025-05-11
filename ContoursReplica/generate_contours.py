import argparse

import numpy as np
from noise import pnoise2
import matplotlib.pyplot as plt
import matplotlib

# 1. Генерация поля высот с Perlin noise
def generate_height_field(width, height, scale=100.0,
                          octaves=6, persistence=0.5,
                          lacunarity=2.0, seed=None):
    field = np.zeros((height, width))
    for y in range(height):
        for x in range(width):
            nx = x / scale
            ny = y / scale
            field[y, x] = pnoise2(nx + seed, ny + seed,
                                  octaves=octaves,
                                  persistence=persistence,
                                  lacunarity=lacunarity,
                                  repeatx=1024,
                                  repeaty=1024)
    # Нормализация в диапазон 0..1
    field = (field - field.min()) / (field.max() - field.min())
    return field

# 2. Построение и отрисовка карты с изолиниями и заливкой

def plot_terrain_map(field, num_levels=12,
                     draw_isolines=True,
                     fill_isolines=True,
                     draw_values=True,
                     cmap_name='RdYlGn_r',
                     output_file=None,
                     output_mask_file=None,
                     dpi=300,
                     verbose=False):
    """
    - field: 2D numpy array высот нормированных [0,1]
    - num_levels: число уровней изолиний
    - cmap_name: цветовая карта для заполнения
    - thick_every: каждая k-я линия толще
    """
    height, width = field.shape
    x = np.arange(width)
    y = np.arange(height)
    X, Y = np.meshgrid(x, y)

    # Уровни
    levels = np.linspace(0.0, 1.0, num_levels)

    fig, ax = plt.subplots(figsize=(10, 6))

    if draw_isolines:
        # 2.1 Заливка областей градиентом посредством contourf
        if fill_isolines:
            contourf = ax.contourf(
                X, Y, field,
                levels=levels,
                cmap=matplotlib.colormaps.get_cmap(cmap_name),
                antialiased=True
            )

        # 2.2 Отрисовка изолиний
        cs = ax.contour(
            X, Y, field,
            levels=levels,
            colors='black',
            linewidths=[0.8
                        for i in range(len(levels))]
        )

        if draw_values:
            # 2.3 Подписи высот вдоль линий
            fmt = {level: f"{round(level*100)}" for level in levels}
            ax.clabel(cs, fmt=fmt, inline=True, fontsize=8, inline_spacing=2)

    ax.set_aspect('equal')
    ax.axis('off')
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=dpi, bbox_inches='tight', pad_inches=0)
        if verbose:
            print(f"Карта сохранена в файл: {output_file}")
    else:
        plt.show()
    
    if output_mask_file:
        fig2, ax2 = plt.subplots(figsize=(10, 6), facecolor='black')
        fig2.patch.set_facecolor('black')
        ax2.set_facecolor('black')

        ax2.contour(X, Y, field, levels=levels,
                    colors='white',
                    linewidths=[0.8
                                for i in range(len(levels))])
        ax2.set_aspect('equal')
        ax2.axis('off')
        plt.tight_layout()
        plt.savefig(output_mask_file, dpi=dpi, bbox_inches='tight', facecolor=fig2.get_facecolor(), pad_inches=0)
        if verbose:
            print(f"Маска изолиний сохранена в файл: {output_mask_file}")
        plt.close(fig2)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Генерация карты рельефа с изолиниями.')
    parser.add_argument('--output', type=str, help='Имя файла для сохранения изображения')
    parser.add_argument('--output_mask', type=str, help='Имя файла для сохранения маски изображения')
    parser.add_argument('--w', type=int, help='Ширина сетки')
    parser.add_argument('--h', type=int, help='Высота сетки')
    parser.add_argument('--dpi', type=int, help='Разрешение рендера')
    parser.add_argument('--draw_isolines', type=int, help='Рисовать изолинии')
    parser.add_argument('--fill_isolines', type=int, help='Заливка изолиний')
    parser.add_argument('--draw_values', type=int, help='Рисовать значения на изолиниях')
    parser.add_argument('-v', '--verbose', action='store_true', help='Логгирование')

    args = parser.parse_args()
    seed = np.random.randint(0, 2**16 - 1)

    verbose = args.verbose

    # Параметры карты
    W, H = args.w, args.h
    dpi = args.dpi
    field = generate_height_field(
        width=W,
        height=H,
        scale=80.0,
        octaves=5,
        persistence=0.6,
        lacunarity=2.0,
        seed=seed
    )
    plot_terrain_map(
        field,
        num_levels=15,
        cmap_name='RdYlGn_r',
        draw_isolines = args.draw_isolines,
        fill_isolines = args.fill_isolines,
        draw_values = args.draw_values,
        output_file=args.output,
        output_mask_file=args.output_mask,
        dpi=dpi,
        verbose=verbose
    )
