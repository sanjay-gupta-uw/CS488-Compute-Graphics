rootNode = gr.node('root')

onigashimaMesh = gr.mesh('onigashima', 'Assets/onigashima.obj')
onigashimaMesh:scale(20, 20, 20)
-- onigashimaMesh:scale(10, 10, 10)

luffyMesh = gr.mesh('strawhat', 'Assets/luffy.obj')
luffyMesh:translate(0, 0, 0)

rootNode:add_child(onigashimaMesh)
rootNode:add_child(luffyMesh)

return rootNode