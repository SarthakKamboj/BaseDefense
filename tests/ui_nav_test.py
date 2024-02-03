import sys
import math

# verts = [
#     (-4,6),
#     (-4,4),
#     (-3,-4),
#     (-3,0),
#     (-2,0),
#     (-1,2),
#     (-1,-2),
#     (0,0),
#     (2,1),
#     (3,4),
#     (4,-4)
# ]

# verts = [
#     (200, 400),
#     (550, 400),
#     (900, 400),
#     (550, 300),
#     (1100, 800)
# ]

verts = [
    (1195.224976, 692.549988), # 0 pause_icon_btn
    (355.169983, 363.491638), # 1 base_container
    (626.700012, 363.491638), # 2 base_ext_container
    (898.229980, 363.491638), # 3 gun_container
    (626.559998, 191.603363), # 4 buy_btn
    (1074.439941, 364.350006), # 5 open_close_section
]

UP = 0
RIGHT = 1
DOWN = 2
LEFT = 3

def dot(a, b):
    return (a[0] * b[0]) + (a[1] * b[1])

def vec_len(a):
    return pow(pow(a[0],2) + pow(a[1],2), 0.5)

def quantize_angle(angle, num_quanta):
    angle_per_quanta = 2 * 3.141526 / num_quanta
    angle += (angle_per_quanta / 2)
    section = min(num_quanta - 1, math.floor(angle / angle_per_quanta))
    return section * angle_per_quanta;

# count = 12
# for i in range(count):
#     print(quantize_angle(math.pi * 2 * i / count, 7))

def normalize(v):
    return (v[0] / vec_len(v), v[1] / vec_len(v))

def go_dir(from_node, dir):

    dir_vec = (0,0)
    if dir == UP:
        dir_vec = (0,1)
    elif dir == RIGHT:
        dir_vec = (1,0)
    elif dir == DOWN:
        dir_vec = (0,-1)
    elif dir == LEFT:
        dir_vec = (-1,0)

    # running = -1
    # running_weight = sys.maxsize

    running_info = {
        "angle": sys.maxsize,
        "distance": sys.maxsize,
        "weight": sys.maxsize,
        "i": -1
    }

    for i in range(len(verts)):
        if i == from_node: continue
        diff = (verts[i][0] - verts[from_node][0], verts[i][1] - verts[from_node][1])
        normalized_diff = normalize(diff)
        dotted = dot(dir_vec, normalized_diff)
        if dotted <= 0: continue

        # weighting
        # weight = vec_len(diff) * dotted

        # angle_weight = angle * angle
        # vec_len_weight = diff_len * diff_len
        # weight = vec_len_weight + angle_weight

        # print(f"possible node dest {i}\n")

        diff_screen_norm = (diff[0] / 1280, diff[1] / 729);
        screen_norm_distance = vec_len(diff_screen_norm);
            
        parallel_to_diff_screen_norm = dot(dir_vec, diff_screen_norm);
        perpen_to_diff_screen_norm_squared = math.pow(screen_norm_distance,2) - math.pow(parallel_to_diff_screen_norm,2);

        pre_quantized_angle = math.acos(dotted);
        quantized_angle = quantize_angle(pre_quantized_angle, 16);

        # weight = screen_norm_distance * screen_norm_distance * parallel_to_diff_screen_norm * 1000;
        # stretching on non-dir axis
        weight = math.pow(parallel_to_diff_screen_norm * 10, 2) + math.pow(perpen_to_diff_screen_norm_squared * 100, 4)

        # print(f"possible node dest: {i}    diff:{str(diff)} dotted:{dotted}  pre_quantized_angle: {str(pre_quantized_angle)}  quantized_angle: {str(quantized_angle)}  screen_norm_distance: {str(screen_norm_distance)}   weight:{str(weight)}  diff_screen_norm:{diff_screen_norm}  parallel_to_diff_screen_norm:{str(parallel_to_diff_screen_norm)}")
        update_running = False
        if (quantized_angle == 0 and running_info["angle"] != 0):
            update_running = True
            # print("updated bc quantized angle is 0\n")
        elif running_info["angle"] == quantized_angle and screen_norm_distance < running_info["distance"]:
            update_running = True
            # print("updated bc norm vec len of diff is less and angle and quantized angle the same")
        elif running_info["angle"] != 0 and quantized_angle != 0 and weight < running_info["weight"]:
            update_running = True
            # print("updated bc running angle and quantized angles not 0 but weight is less")


        # print("at node " + str(i) + " diff was " + str(diff) + " and weight was " + str(weight))
        if update_running:
            # print("\n")
            # print("updated\n")
            running_info["angle"] = quantized_angle
            running_info["weight"] = weight
            running_info["distance"] = screen_norm_distance
            running_info["i"] = i
            # running = i
            # running_weight = weight
            # print("node " + str(i) + " was selected which is at " + str(verts[i]))
    return running_info["i"]
    
for i in range(len(verts)):
# for i in [0]:
    # print("node " + str(i))
    for j in range(4):
        dir_str = ""
        if j == 0:
            dir_str = "UP"
        elif j == 1:
            dir_str = "RIGHT"
        elif j == 2:
            dir_str = "DOWN"
        elif j == 3:
            dir_str = "LEFT"
        # print(dir_str + "\n")
        dest = go_dir(i, j)
        print("go " + dir_str + " for node " + str(i) + " is " + str(dest))
    print("\n")