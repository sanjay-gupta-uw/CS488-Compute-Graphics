-- A simple scene with some miscellaneous geometry.

mat1 = gr.material({0.7, 1.0, 0.7}, {0.5, 0.7, 0.5}, 25)
mat2 = gr.material({0.5, 0.5, 0.5}, {0.5, 0.7, 0.5}, 25)
mat3 = gr.material({1.0, 0.6, 0.1}, {0.5, 0.7, 0.5}, 25)
mat4 = gr.material({0.7, 0.6, 1.0}, {0.5, 0.4, 0.8}, 25)

scene_root = gr.node('root')
scene_root:scale(0.5, 0.5, 0.5)
scene_root:translate(0, 100, 0)

-- s1 = gr.nh_sphere('s1', {-300, 0, -400}, 100)
-- scene_root:add_child(s1)
-- s1:set_material(mat1)
-- s1:scale(1, 2, 1)

-- s2 = gr.sphere('s2') 
-- scene_root:add_child(s2)
-- s2:set_material(mat4)
-- -- s2:rotate('x', 140)
-- s2:scale(100, 100, 100)
-- s2:translate(0, -300, -400)


-- b1 = gr.cube('b1')
-- -- b1 = gr.nh_box('b1', {0, 0, -100}, 100)
-- scene_root:add_child(b1)
-- b1:set_material(mat4)
-- b1:scale(100, 100, 100)
-- b1:translate(0, 0, 100)
-- b1:scale(1, 2, 1)
-- b1:rotate('y', 45)


steldodec = gr.mesh( 'dodec', 'smstdodeca.obj' )
steldodec:set_material(mat3)
scene_root:add_child(steldodec)
-- steldodec:scale(2, 2, 2)
steldodec:translate(0, -300, 0)
steldodec:rotate('y', 45)


white_light = gr.light({-100.0, 150.0, 400.0}, {0.9, 0.9, 0.9}, {1, 0, 0})
magenta_light = gr.light({400.0, 100.0, 150.0}, {0.7, 0.0, 0.7}, {1, 0, 0})

gr.render(scene_root, 'sample.png', 512, 512,
	  {0, 0, 800}, {0, 0, -1}, {0, 1, 0}, 50,
	  {0.3, 0.3, 0.3}, {white_light, magenta_light})
