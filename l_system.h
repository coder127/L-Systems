
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

namespace octet {

    /// Scene 
    class l_system : public app {

        // scene for drawing box
        ref<visual_scene> app_scene;
        ref<text_overlay> overlay;
        ref<mesh_text> text;

        ifstream config_file;

        int iterations, original_iterations, config_number = 1, array_top = 0, mesh_counter;
        int season = 3; //1 = winter, 2 = spring,  3 = summer, 4 = autumn

        float angle, cam_max_x = 0, cam_min_x = 0, cam_max_y = 0, cam_min_y = 0, x_position, y_position, z_position; //variables for the camera 
        float branch_size = 5.0f; //size of the cylinders

        std::string axiom, drawing_rule, rule1, rule2, rule3; //variables used in program, taken from file
        std::string line_iterations, line_angle, line_axiom, line_rule1, line_rule2, line_rule3, season_word; //variables read from file

        mat4t modelToWorld;
        mat4t branches[2048]; //stack of matrices to push and pop as per the production rules

        scene_node parent; //the first cylinder, parent to all cylinders following

        material *wood, *red, *green, *white, *orange, *yellow, *pink; //the textures

        dynarray<scene_node*> nodes;

        bool plant = true, stochastic = false; //plant is true if the configuration is for a plant, false otherwise. same applies for stochastic
        bool show_text = true;

        random rand; //for stochastic configuration

        mesh *mesh_branch;
        mesh *mesh_leaf;
        mesh *mesh_pistil;

        public:
        /// this is called when we construct the class before everything is initialised.
        l_system(int argc, char **argv) : app(argc, argv) {
            }

        //open the config file, read each line and set variables
        void load_file(std::string file) {
            config_file.open(file);
            getline(config_file, line_iterations);
            getline(config_file, line_angle);
            getline(config_file, line_axiom);
            getline(config_file, line_rule1);
            getline(config_file, line_rule2);
            getline(config_file, line_rule3);
            config_file.close();

            iterations = ::atoi(line_iterations.c_str());
            original_iterations = iterations;
            angle = ::atof(line_angle.c_str());
            axiom = line_axiom;
            rule1 = line_rule1;
            rule2 = line_rule2;
            rule3 = line_rule3;

            rule1.erase(0, 3); rule2.erase(0, 3); rule3.erase(0, 3); //erase the three beginning characters eg. 'F->'
            }

        //make a new cylinder (mesh) of a certain (texture) and make it a child of (parent) add it to the scene.
        void draw_branch(mesh *_mesh, material texture, scene_node *parent){

            material *mat = new material(texture);
            scene_node *node = new scene_node(modelToWorld, atom_);
            nodes.push_back(node);


            //begin by making the first cylinder a child of the scene
            if (nodes.size() == 1){
                app_scene->add_child(node);
                parent = node;
                }

            //then every other cylinder a child of the first cylinder - to be used when rotating final tree
            else { parent->add_child(node); }

            //rotate the cylinder to facing up position
            node->rotate(90, vec3(1.0f, 0.0f, 0.0f));

            app_scene->add_mesh_instance(new mesh_instance(node, _mesh, mat));

            mesh_counter++; //keeps track of how many cylinders have been drawn
            }

        //puts the current details of matrix into an array of matrices
        void push_matrix(mat4t matrix){
            branches[array_top] = matrix; //put the current state of modelToWorld into the top of the matrix stack
            array_top++;
            }

        //retrieves the matrix position that were stored in the array of matrices
        mat4t pop_matrix(){
            return branches[--array_top]; //bring back the pushed matrix
            }

        void simulate(material tree, material pistil, material petal){
            //reset everything and set values
            nodes.reset();
            mesh_counter = 0;
            cam_max_x = 0; cam_max_y = 0; cam_min_x = 0; cam_min_y = 0;
            modelToWorld.loadIdentity();
            drawing_rule = axiom;
            dynarray<int> index;
            mesh_branch = new mesh_cylinder(zcylinder(vec3(0.0f, 0.0f, 0.0f), 1.5f, branch_size));
            mesh_leaf = new mesh_cylinder(zcylinder(vec3(0.0f, 0.0f, 0.0f), 1.5f, 0.5f));
            mesh_pistil = new mesh_sphere(vec3(0.0f, 0.0f, 0.0f), 1.5f);

            //for the amount of iterations specified...
            for (int i = 0; i < iterations; i++){
                //after initial iteration, run through the rule string and look for letters to be replaced and push their indexes into an array.
                if (i > 0){
                    for (int j = 0; j < drawing_rule.length(); j++){
                        if (plant == true){
                            if (drawing_rule[j] == 'F'){
                                index.push_back(j);
                                }
                            }
                        if (drawing_rule[j] == 'X'){
                            index.push_back(j);
                            }
                        if (plant == false){
                            if (drawing_rule[j] == 'Y'){
                                index.push_back(j);
                                }
                            }
                        }
                    //then for each index specifed replace the letter with the rule specified. LOOP MUST RUN BACKWARDS.
                    for (int k = index.size() - 1; k >= 0; k--){
                        if (stochastic == false){
                            if (drawing_rule[index[k]] == 'F'){
                                drawing_rule.replace(index[k], 1, rule1);
                                }
                            else if (drawing_rule[index[k]] == 'X'){
                                drawing_rule.replace(index[k], 1, rule2);
                                }
                            else if (drawing_rule[index[k]] == 'Y'){
                                drawing_rule.replace(index[k], 1, rule1);
                                }
                            }
                        if (stochastic == true){
                            int number = rand.get(0, 100);
                            if (number <= 33){
                                drawing_rule.replace(index[k], 1, rule1);
                                }
                            else if (number > 33 && number <= 66) {
                                drawing_rule.replace(index[k], 1, rule2);
                                }
                            else if (number > 66 && number <= 100) {
                                drawing_rule.replace(index[k], 1, rule3);
                                }
                            }
                        }
                    index.reset();
                    }

                //then just go through each letter in the drawing rule string and do action associated with it
                for (int l = 0; l <= drawing_rule.length(); l++){
                    //create a cylinder and move up
                    if (drawing_rule[l] == 'F'){
                        draw_branch(mesh_branch, tree, &parent);
                        modelToWorld.translate(vec3(0, (branch_size + (branch_size / 2)), 0));
                        }
           
                    if (drawing_rule[l] == '+'){
                        modelToWorld.rotate(angle * 4, 0.0f, 1.0f, 0.0f); //positive rotate in the y-axis
                        modelToWorld.rotate(angle, 0.0f, 0.0f, 1.0f); //positive rotate in the z z-axis
                        }

                    else if (drawing_rule[l] == '-'){
                        modelToWorld.rotate(angle * 4, 0.0f, 1.0f, 0.0f); //negative rotate in the y-axis
                        modelToWorld.rotate(-angle, 0.0f, 0.0f, 1.0f); //negative rotate in the z-axis
                        }

                    //push the current state of the matrix
                    if (drawing_rule[l] == '['){
                        push_matrix(modelToWorld);
                        }
                    //draw leaves and pop the stack back
                    else if (drawing_rule[l] == ']'){
                        draw_branch(mesh_pistil, pistil, &parent); //first draw the pistil (sphere at end of branch)
                            modelToWorld.rotate(45, 1.0f, 0.0f, 0.0f); //the rest creates a flower-like structure of cylinders to symbolise leaves.
                            modelToWorld.translate(vec3(0.0f, 0.0f, 1.0f));
                            draw_branch(mesh_leaf, petal, &parent);
                            for (int q = 0; q < 4; q++){
                                for (int p = 0; p < 3; p++){
                                    modelToWorld.rotate(72, 1.0f, 0.0f, 0.0f);
                                    modelToWorld.translate(vec3(2.0, 0.0f, 0.0f));
                                    draw_branch(mesh_leaf, petal, &parent);
                                    }
                                modelToWorld.translate(vec3(-8.0f, 0.0f, 0.0f));
                                modelToWorld.rotate(90, 0.0f, 1.0f, 0.0f);
                                }
                        modelToWorld = pop_matrix();
                        }
                    }
                }
            //for every cylinder that has been drawn, get the highest one, the lowest one, the left and the right-most ones and set the camera in accordance
            for (int i = 0; i < mesh_counter; i++){
                vec3 position = app_scene->get_mesh_instance(i)->get_node()->get_position();
                if (position.y() > cam_max_y){
                    cam_max_y = position.y();
                    }
                if (position.x() > cam_max_x){
                    cam_max_x = position.x();
                    }
                if (position.x() < cam_min_x){
                    cam_min_x = position.x();
                    }
                if (position.y() < cam_min_y){
                    cam_min_y = position.y();
                    }
                }
            x_position = ((cam_max_x + cam_min_x) / 2);
            y_position = ((cam_max_y + cam_min_y) / 2);
            z_position = ((cam_max_y - cam_min_y) * 1.5);
            app_scene->get_camera_instance(0)->get_node()->translate(vec3(x_position, y_position, z_position));

            }

        //hotkey controller
        void controller(){

            scene_node *node = app_scene->get_camera_instance(0)->get_node();

            if (is_key_down(key_up)){
                if (iterations < original_iterations){
                    iterations++;
                    run();
                    }
                }

            else if (is_key_down(key_down)){
                if (iterations > 1){
                    iterations--;
                    run();
                    }
                }

            else if (is_key_down(key_left)){
                if (angle > 10){
                    angle -= 5;
                    run();
                    }
                }

            else if (is_key_down(key_right)){
                if (angle < 170){
                    angle += 5;
                    run();
                    }
                }

 
            else if (is_key_down('F')){
                rule1.insert(rule1.end(), '+'); line_rule1.insert(line_rule1.end(), '+');
                rule1.insert(rule1.end(), 'F'); line_rule1.insert(line_rule1.end(), 'F');
                run();
                }

            else if (is_key_down('V')){
                rule1.insert(rule1.end(), '-'); line_rule1.insert(line_rule1.end(), '-');
                rule1.insert(rule1.end(), 'F'); line_rule1.insert(line_rule1.end(), 'F');
                run();
                }

            else if (is_key_down('M')){
                season += 1;
                if (season == 5) { season = 1; }
                run();
                }


            else if (is_key_down('S')){
                if (node->access_nodeToParent().w().z() <= z_position){
                    node->translate(vec3(0.0f, 0.0f, 50.0f));
                    }
                }

            else if (is_key_down('W')){
                if (node->access_nodeToParent().w().z() >= 100){
                    node->translate(vec3(0.0f, 0.0f, -50.0f));
                    }
                }

            else if (is_key_down('P')){
                  show_text = false;
                }

            else if (is_key_down('L')){
                show_text = true;
                }

            else if (is_key_down('1')){
                plant = true;
                stochastic = false;
                config_number = 1;
                std::string file = "config/1.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('2')){
                plant = true;
                stochastic = false;
                config_number = 2;
                std::string file = "config/2.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('3')){
                plant = true;
                stochastic = false;
                config_number = 3;
                std::string file = "config/3.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('4')){
                plant = true;
                stochastic = false;
                config_number = 4;
                std::string file = "config/4.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('5')){
                plant = true;
                stochastic = false;
                config_number = 5;
                std::string file = "config/5.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('6')){
                plant = true;
                stochastic = false;
                config_number = 6;
                std::string file = "config/6.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('7')){
                plant = false;
                stochastic = false;
                config_number = 7;
                std::string file = "config/7.txt";
                load_file(file);
                run();
                }

            else if (is_key_down('8')){
                plant = true;
                stochastic = true;
                config_number = 8;
                std::string file = "config/8.txt";
                load_file(file);
                run();
                }
            }

        //create the initial app_scene - used each time a new config file is loaded 
        void make_scene(){
            app_scene = new visual_scene();
            app_scene->create_default_camera_and_lights();
            app_scene->get_camera_instance(0)->set_far_plane(10000);
            app_scene->get_camera_instance(0)->set_near_plane(1);
            }

        //reset the scene, create a new one, and run again
        void run(){
            app_scene->reset();
            make_scene();
            if (season == 1){
                season_word = "Winter";
                simulate(*wood, *white, *white);
                }
            else if (season == 2){
                season_word = "Spring";
                simulate(*wood, *yellow, *pink);
                }
            else if (season == 3){
                season_word = "Summer";
                simulate(*wood, *red, *green);
                }
            else if (season == 4){
                season_word = "Autumn";
                simulate(*wood, *wood, *orange);
                }
            }

        //creates a new mesh text in a certain size text box
        void create_text(){
            overlay = new text_overlay();
            bitmap_font *font = overlay->get_default_font();
            aabb text_box(vec3(-50.0f, -150.0f, 0.0f), vec3(300, 150, 0));
            text = new mesh_text(font, "", &text_box);
            overlay->add_mesh_text(text);
            }

        //shows what text to be shown on screen
        void draw_text(){

            if (show_text == true){
                text->clear();
                text->format(
                    "Season: %s\n"
                    "Configuration: %d\n"
                    "Iterations: %d   (A limit has been set to a maximum of '%d' iterations)\n"
                    "Angle: %g\n"
                    "Axiom: %s\n"
                    "Rule 1: %s\n"
                    "Rule 2: %s\n"
                    "Rule 3: %s\n"
                    "Press the 'NUMBER KEYS' to select configuration file\n"
                    "Press the 'UP' and 'DOWN' keys to change the number of iterations\n"
                    "Press the 'LEFT' and 'RIGHT' keys to change the angle\n"
                    "Press the 'F' and 'V' keys to add a '+F' or a '-F' to rule 1, respectively\n"
                    "Press the 'M' key to  loop through the seasons\n"
                    "Press the 'S' and 'W' keys to zoom in and out\n"
                    "Press the 'P' and 'L' keys to toggle the text on and off\n"
                    "Configuration '7' is the 'Dragon Curve' fractal\n"
                    "Configuration '8' is stochastic so will display a different tree each time it is run",
                    season_word.c_str(),
                    config_number,
                    iterations,
                    original_iterations,
                    angle,
                    axiom.c_str(),
                    line_rule1.c_str(),
                    line_rule2.c_str(),
                    line_rule3.c_str()
                    );
                }

            else if (show_text == false){
                text->clear();
                text->format("Press the 'L' key to bring the text back");
                }
            }
        void app_init() {
            
            make_scene();

            create_text();

            //all materials to be used are loaded in once. 
            wood = new material(new image("C:/Users/Tahmoor/Documents/Maths/octet/octet/src/examples/l_system/texture/wood.gif"));
            red = new material(vec4(1.0f, 0.0f, 0.0f, 1.0f));
            green = new material(vec4(0.0f, 0.5f, 0.0f, 1.0f));
            pink = new material(vec4(1.0f, 0.71f, 0.76f, 1.0f));
            yellow = new material(vec4(1.0f, 1.0f, 0.0f, 1.0f));
            white = new material(vec4(1.0f, 1.0f, 1.0f, 1.0f));
            orange = new material(vec4(0.8f, 0.5f, 0.0f, 1.0f));

            //starting file is the first config file.
            load_file("config/1.txt");
            season_word = "Summer";
            simulate(*wood, *red, *green);
            }

        // this is called to draw the world
        void draw_world(int x, int y, int w, int h) {

            int vx = 0, vy = 0;
            get_viewport_size(vx, vy);
            app_scene->begin_render(vx, vy);

            //gets hotkey input
            controller();

            //rotate the L-System automatically. 
            parent.rotate(3.0f, vec3(0.0f, 1.0f, 0.0f));

            // update matrices. assume 30 fps.
            app_scene->update(1.0f / 30);

            // draw the scene
            app_scene->render((float)vx / vy);
            
            //draw the text on the screen
            draw_text();

            // convert it to a mesh.
            text->update();
            // draw the text overlay
            overlay->render(vx, vy);
            }
        };
    }
