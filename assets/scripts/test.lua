local fox = Entity.new()
local t = fox:add(Transform.new())
t.scale.x = 0.1
t.scale.y = 0.1
t.scale.z = 0.1
t.position.y = -30
fox:add(Renderable.new(assets.models['fox']))
fox:add(SkeletalAnimation.new(assets.models['fox'], assets.animations['fox_run']))

