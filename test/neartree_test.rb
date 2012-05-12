require 'neartree'
require 'test/unit'

class NearTreeRTreeTest < Test::Unit::TestCase

	def setup
		@points = [
			[[0, 0, 0],       "point 0"],
			[[10.0, 1, 0.5],  "point 1"],
			[[1.0, 1.0, 1.0], "point 2"]]


		@bad_indices = [
			['some arg',         TypeError    ],
			[1,                  TypeError    ],
			[[],                 ArgumentError],
 		 	[[1,2,3,4],          ArgumentError],
			[[1.0, 0.0, 'blah'], TypeError    ]]

		@good_index = [0.0, 0.0, 0.0]

		@bad_radii = [
			['blah'   , TypeError    ],
			[[1, 2, 3], TypeError    ],
 		  [{}       , TypeError    ],
 		  [-3.4     , ArgumentError]]

		@good_radius = 1.0

		@bad_ks = [
			['blah', TypeError    ],
			[0     , ArgumentError],
			[-4    , ArgumentError],
			[3.5   , TypeError    ],
			[[1, 2], TypeError    ],
			[[3]   , TypeError    ],
			[{}    , TypeError    ]]
	end

	# initialize
	def test_initialize
		# bad dimension types
		assert_raise(TypeError){ NearTree::RTree.new("blah") }
		assert_raise(TypeError){ NearTree::RTree.new(3.4)    }

		# bad dimension values
		assert_raise(ArgumentError){ NearTree::RTree.new(0)  }
		assert_raise(ArgumentError){ NearTree::RTree.new(-3) }

		# state after initialisation for various dimensions
		(1..32).each do |dimension|
			tree = NearTree::RTree.new(dimension)
			assert_equal(dimension, tree.dimension)
			assert_equal(0,         tree.points.size)
			assert_equal(0,         tree.values.size)
		end
	end

	# insert
	def test_insert
		# setup
		tree = NearTree::RTree.new(3)

		# bad indices
		@bad_indices.each{ |i, e| assert_raise(e){ tree.insert(i, 0) } }

		@points.each_with_index do |p, i|
			assert_equal(nil, tree.insert(p[0], p[1]))

			# check points/values size/content
			assert_equal(i + 1, tree.points.size)
			assert_equal(i + 1, tree.values.size)
			assert_equal(p[0], tree.points.last)
			assert_equal(p[1], tree.values.last)
		end
	end

	# find_nearest
	def test_find_nearest
		setup_find

		@bad_indices.each{ |index , e|
			assert_raise(e){ @tree.find_nearest(index,       @good_radius) } }
		@bad_radii.each{   |radius, e|
			assert_raise(e){ @tree.find_nearest(@good_index, radius      ) } }

		[nil, 0.0, 100].each{ |radius|
			@points.each{ |k, v| assert_equal([k, v], @tree.find_nearest(k, radius)) } }

		assert_equal(@points[0], @tree.find_nearest([0.0, 0.0, 1.0]     ))
		assert_equal(@points[0], @tree.find_nearest([0.0, 0.0, 1.0], nil))
		assert_raise(KeyError){  @tree.find_nearest([0.0, 0.0, 1.0], 0.5) }
	end

	# find_k_nearest
	def test_find_k_nearest
		setup_find

		@bad_indices.each{ |index , e|
			assert_raise(e){ @tree.find_k_nearest(index,       1, @good_radius) } }
		@bad_ks.each{      |k     , e|
			assert_raise(e, "Bad k #{k}"){ @tree.find_k_nearest(@good_index, k      , @good_radius) } }
		@bad_radii.each{   |radius, e|
			assert_raise(e, "Bad radius #{radius}"){
				@tree.find_k_nearest(@good_index, 1, radius      ) } }

		[nil, 0.0, 100].each{ |radius|
			@points.each{ |c, v| assert_equal([[c, v]], @tree.find_k_nearest(c, 1, radius)) } }

		assert_equal([@points[0], @points[2], @points[1]], @tree.find_k_nearest(@points[0][0], 3, 100))
		assert_equal([@points[1], @points[2], @points[0]], @tree.find_k_nearest(@points[1][0], 3, 100))
		assert_equal([@points[2], @points[0], @points[1]], @tree.find_k_nearest(@points[2][0], 3, 100))

		assert_equal([@points[0], @points[2]], @tree.find_k_nearest(@points[0][0], 3, 2))
		assert_equal([@points[1]            ], @tree.find_k_nearest(@points[1][0], 3, 1))
		assert_equal([@points[2], @points[0]], @tree.find_k_nearest(@points[2][0], 3, 2))

		assert_raise(KeyError){ @tree.find_k_nearest([0.0, 0.0, 1.0], 2, 0.5) }
	end
	
private
	# setup for find_nearest, find_k_nearest
	def setup_find
		@tree = NearTree::RTree.new(3)

		@points.each_with_index{ |p, i| @tree.insert(p[0], p[1]) }
	end
end # NearTreeRTreeTest

