red = gr.material({1.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10)
blue = gr.material({0.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)
green = gr.material({0.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
white = gr.material({1.0, 1.0, 1.0}, {0.1, 0.1, 0.1}, 10)

rootNode = gr.node('root')

headMesh = gr.mesh('luffy-face', 'head')
headMesh:scale(1.2, 1.2, 1.2)
headMesh:set_material(red)

neckMesh = gr.mesh('neck', 'neck')
neckMesh:translate(0.0, 0.11, 0.03)
neckMesh:scale(1.2, 1.2, 1.2)
neckMesh:set_material(green)

torsoMesh = gr.mesh('torso_mesh', 'torso')
torsoMesh:scale(1.2, 1.2, 1.2)
torsoMesh:set_material(blue)

rightUpperarmMesh = gr.mesh('right-upperarm', 'right_upperarm')
rightUpperarmMesh:scale(1.2, 1.2, 1.2)
rightUpperarmMesh:set_material(red)

leftUpperarmMesh = gr.mesh('left-upperarm', 'left-upperarm')
leftUpperarmMesh:scale(1.2, 1.2, 1.2)
-- leftUpperarmMesh:rotate('y', -50)
leftUpperarmMesh:set_material(red)

rightForearmMesh = gr.mesh('right-forearm', 'right_forearm')
rightForearmMesh:scale(1.2, 1.2, 1.2)
rightForearmMesh:set_material(green)

leftForearmMesh = gr.mesh('left-forearm', 'left_forearm')
leftForearmMesh:scale(1.2, 1.2, 1.2)
leftForearmMesh:set_material(green)

rightHandMesh = gr.mesh('right-hand', 'right_hand')
rightHandMesh:scale(1.2, 1.2, 1.2)
rightHandMesh:set_material(blue)

leftHandMesh = gr.mesh('left-hand', 'left_hand')
leftHandMesh:scale(1.2, 1.2, 1.2)
leftHandMesh:set_material(blue)

rightThighMesh = gr.mesh('right-thigh', 'right_thigh')
rightThighMesh:scale(1.2, 1.2, 1.2)
rightThighMesh:set_material(red)

leftThighMesh = gr.mesh('left-thigh', 'left_thigh')
leftThighMesh:scale(1.2, 1.2, 1.2)
leftThighMesh:set_material(red)

rightShinMesh = gr.mesh('right-calf', 'right_shin')
rightShinMesh:scale(1.2, 1.2, 1.2)
rightShinMesh:set_material(green)

leftShinMesh = gr.mesh('left-calf', 'left_shin')
leftShinMesh:scale(1.2, 1.2, 1.2)
leftShinMesh:set_material(green)

rightFootMesh = gr.mesh('right-foot', 'right_foot')
rightFootMesh:scale(1.2, 1.2, 1.2)
rightFootMesh:set_material(blue)

leftFootMesh = gr.mesh('left-foot', 'left_foot')
leftFootMesh:scale(1.2, 1.2, 1.2)
leftFootMesh:set_material(blue)


-- joints
-- should be 3 head neck joints
Neck_Head_Joint = gr.joint('head-neck-joint', {-40, 0, 50}, {-90, 0, 90})
Neck_Head_Joint:translate(0.0, 0.13, 0.04)

Neck_Torso_Joint = gr.joint('neck-torso-joint', {0, 0, 0}, {-20, 0, 20})

RightShoulderJoint = gr.joint('RightShoulderJoint', {0, 0, 0}, {0, 0, 80})
RightShoulderJoint:translate(-0.35, -0.23, -0.01)
-- RightShoulderJoint:rotate('y', 80)
LeftShoulderJoint = gr.joint('LeftShoulderJoint', {0, 0, 0}, {-80, 0, 0})
-- LeftShoulderJoint:rotate('y', -80)
LeftShoulderJoint:translate(0.38, -0.23, -0.022)

RightElbowJoint = gr.joint('RightElbowJoint', {0, 0, 0}, {0, 0, 90})
RightElbowJoint:translate(-0.82, -0.10, 0.06)
LeftElbowJoint = gr.joint('LeftElbowJoint', {0, 0, 0}, {-90, 0, 0} )
LeftElbowJoint:translate(0.8, -0.02, 0.24)

RightWristJoint = gr.joint('RightWristJoint', {0, 0, 0}, {-25, 0, 25})
RightWristJoint:translate(-0.505, -0.015, 0.19)
LeftWristJoint = gr.joint('LeftWristJoint', {0, 0, 0}, {-25, 0, 25})
LeftWristJoint:translate(0.43, 0.03, 0.28)

RightHipJoint = gr.joint('RightHipJoint', {-88, 0, 70}, {0, 0, 0})
RightHipJoint:translate(-0.13, -1.08, 0.0)
LeftHipJoint = gr.joint('LeftHipJoint', {-88, 0, 70}, {0, 0, 0})
LeftHipJoint:translate(0.13, -1.08, -0.03)

RightKneeJoint = gr.joint('RightKneeJoint', {0, 0, 90}, {0, 0, 0})
RightKneeJoint:translate(-0.145, -0.96, 0.015)
LeftKneeJoint = gr.joint('LeftKneeJoint', {0, 0, 90}, {0, 0, 0})
LeftKneeJoint:translate(0.24, -0.94, 0.066)

RightAnkleJoint = gr.joint('RightAnkleJoint', {-20, 0, 60}, {0, 0, 0})
RightAnkleJoint:translate(-0.05, -0.77, -0.14)
LeftAnkleJoint = gr.joint('LeftAnkleJoint', {-20, 0, 60}, {0, 0, 0})
LeftAnkleJoint:translate(0.15, -0.76, -0.12)

-- build the hierarchy
rootNode:add_child(neckMesh)
rootNode:add_child(Neck_Torso_Joint)
rootNode:add_child(Neck_Head_Joint)
Neck_Torso_Joint:add_child(RightShoulderJoint)
Neck_Torso_Joint:add_child(RightHipJoint)
Neck_Torso_Joint:add_child(LeftHipJoint)
Neck_Torso_Joint:add_child(LeftShoulderJoint)

Neck_Head_Joint:add_child(headMesh)
Neck_Torso_Joint:add_child(torsoMesh)

RightShoulderJoint:add_child(rightUpperarmMesh)
RightShoulderJoint:add_child(RightElbowJoint)
RightElbowJoint:add_child(rightForearmMesh)
RightElbowJoint:add_child(RightWristJoint)
RightWristJoint:add_child(rightHandMesh)

LeftShoulderJoint:add_child(leftUpperarmMesh)
LeftShoulderJoint:add_child(LeftElbowJoint)
LeftElbowJoint:add_child(leftForearmMesh)
LeftElbowJoint:add_child(LeftWristJoint)
LeftWristJoint:add_child(leftHandMesh)


RightHipJoint:add_child(rightThighMesh)
RightHipJoint:add_child(RightKneeJoint)
RightKneeJoint:add_child(rightShinMesh)
RightKneeJoint:add_child(RightAnkleJoint)
RightAnkleJoint:add_child(rightFootMesh)

LeftHipJoint:add_child(leftThighMesh)
LeftHipJoint:add_child(LeftKneeJoint)
LeftKneeJoint:add_child(leftShinMesh)
LeftKneeJoint:add_child(LeftAnkleJoint)
LeftAnkleJoint:add_child(leftFootMesh)

rootNode:translate(0.0, 1.0, -5.0)
-- rootNode:rotate('y', -180) -- add rotation to the root node

return rootNode