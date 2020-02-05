#include <fstream>
#include <cfloat>
#include "bvh.h"
#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include "moving_sphere.h"
#include "sphere.h"
#include "texture.h"
#include <ctime>
#include <cstdio>

/* 【功能】获取当前打印像素点所需颜色色值。 */
/*
    接受参数: <观察者光线>，<所有可命中物体组构成的世界>，<递归层数>。
    对散射进行递归，
*/
vec3 color(const ray &r, hittable *world, int depth)
{
    hit_record rec;
    bool hitanything = world->hit(r, 0.001, FLT_MAX, rec);
    if (hitanything)   // 光线物体有交点
    {
        ray scattered;      // 散射/反射后光线
        vec3 attenuation;   // 衰减
        // 手动设置递归层数小于50层，并发生散射/反射后，用新产生的光线继续emmmm...射出去撞东西？最后各向综合起来的才是最终观察到的颜色。
        // 递归层数是防止光线在一个只有反射面的盒子里，递归到爆
        // int material_monitor = rec.mat_ptr->print_material();
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            return attenuation * color(scattered, world, depth+1);  // 根据命中的材质特性，光线被吸收多少，相应衰减多少强度
        }
        else // 层数到了或者衰减到一定程度就莫有了，(0, 0, 0)黑色
        {
            return vec3(0, 0, 0);
        }
    }
    else    // 无交点
    {
        // 那就是原来的背景色！
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5*(unit_direction.y() + 1.0);
        return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
    }
}

hittable *random_scene()
{
    int n = 500;    // 要生成的球个数
    hittable **list = new hittable*[n+1]; // 开链表存储
    texture *checker = new checker_texture( new constant_texture(vec3(0.2, 0.3, 0.1)), new constant_texture(vec3(0.9, 0.9, 0.9)));
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker)); // 底下放个漫反射大球当地板，大球棋盘纹理
    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float choose_mat = random_double();   // 生成随机数决定当前产生的小球材质
            /*
                为了把小球都显示在大球上面，高度y差不多定在0.2。
                x和z坐标在(-11, 11)的样子，0.9*(0,1)=(0,0.9)
                而(4, 0.2, 0)应该是(4,1,0)的镜面反射大球和当地板的球接触的地方，避免有小球黏在那里不好看所以加这个限制吧。
            */
            vec3 center(a+0.9*random_double(), 0.2, b+0.9*random_double());
            if ((center-vec3(4, 0.2, 0)).length() > 0.9)
            {
                if (choose_mat < 0.8)   // 漫反射材质。让哑光球移动。
                {
                    list[i++] = new moving_sphere( center, center + vec3(0, 0.5*random_double(), 0), 
                                                    0.0, 1.0, 0.2, 
                                                    new lambertian(new constant_texture(vec3(random_double()*random_double(), random_double()*random_double(), random_double()*random_double()))));
                }
                else if (choose_mat < 0.95) // 镜面反射材质
                {
                    list[i++] = new sphere(center, 0.2, new metal(vec3(0.5*(1+random_double()), 0.5*(1+random_double()), 0.5*(1+random_double())),0.5*random_double()));
                }
                else    // 玻璃球
                {
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    // 固定参数生成中间的三个大球
    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));    // 中间玻璃球
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4, 0.2, 0.1))));   // 里面漫反射球
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));    // 靠外镜面反射球

    // return new hittable_list(list, i);
    return new bvh_node(list, i, 0.0, 0.5);
}

hittable *two_sphere()
{
    texture *checker = new checker_texture( new constant_texture(vec3(0.2, 0.3, 0.1)), new constant_texture(vec3(0.9, 0.9, 0.9)));
    int n = 5;
    
    hittable **list = new hittable*[n+1];
    list[0] = new sphere(vec3(0, -10, 0), 10, new lambertian( checker ));
    list[1] = new sphere(vec3(0,  10, 0), 10, new lambertian( checker ));

    return new hittable_list(list, 2);
}

int main()
{
    clock_t start, end;
    start = clock();

    ofstream output;
    output.open("chapter3-4-twosphere.ppm");
    
    if(output.is_open()) printf("open file ok\n");

    int nx = 1280, ny = 720 , ns = 10;
    output << "P3\n" << nx << " " << ny << "\n255\n";

    end = clock();
    printf("test time: %fs\n",(double)(end-start)/CLOCKS_PER_SEC);

    printf("\n[create bvh tree]\n");
    // hittable *world = random_scene();
    hittable *world = two_sphere();
    printf("\n[world create ok]\n");

    vec3 lookfrom(13,2,3);
    vec3 lookat(0,0,0);
    float dist_to_focus = 10.0;
    float aperture = 0.0;
    float vfov = 20.0;
    camera cam( lookfrom, lookat, vec3(0,1,0), 
                vfov, float(nx)/float(ny), 
                aperture, dist_to_focus, 
                0.0, 0.5);

    printf("camera create ok\n");

    for (int j = ny-1; j >= 0; j--)
    {
        for (int i = 0; i < nx; i++)
        {
            vec3 col(0, 0, 0);  // 针对每个像素点，初始色值0
            // printf("pos: (%d %d)\n", i, j);
            for(int s = 0; s < ns; s++) // 每个像素点中用100束光线进行采样
            {
                // 光线方向在camera中已设定好，这里生成的就是在成像平面上相对左下角，在宽高两方向上偏移的系数
                float offsetu = float(i + random_double()) / float(nx);
                float offsetv = float(j + random_double()) / float(ny);
                // 对当前的采样值生成光线并映射到相应色值，累加
                ray r = cam.get_ray(offsetu, offsetv);
                col += color(r, world, 0);
            }
            // printf("\n");
            col /= float(ns);   // 最后取均值
            // "Gamma 2"，开1/gamma次根号，这里即开根号
            col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]) );
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);

            output << ir << " " << ig << " " << ib << "\n";
            // printf("i=%d, j=%d\n", i, j);
        }
    }

    output.close();

    end = clock();
    double second = (double)(end-start) / CLOCKS_PER_SEC;
    printf("Run time: %fs\n", second);

    system("pause");
    return 0;
}