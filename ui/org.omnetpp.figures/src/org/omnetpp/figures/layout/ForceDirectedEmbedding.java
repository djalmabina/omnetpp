package org.omnetpp.figures.layout;

import java.util.Random;

import org.eclipse.core.runtime.Assert;

public class ForceDirectedEmbedding extends GraphEmbedding
{
	protected static boolean debug = false;

	protected Random random;

	/**
	 * Position change listener gets notification during the solution.
	 */
	protected IForceDirectedEmbeddingListener listener;

	/**
	 * Spring between vertices having connections.
	 */
	public double connectionSpringCoefficient = 0.1;

	/**
	 * Spring between two vertexs.
	 */
	public double vertexSpringCoefficient = 0.001;

	/**
	 * Maximum spring force.
	 */
	public double maxSpringForce = 1000;

	/**
	 * Magnetism and repeal between two connections' endings.
	 */
	public double connectionMagneticFieldCoefficient = 0;// 1000;
	public Pt connectionHomogenousMagneticFieldPower = new Pt(0, 0.5);
	public double connectionMagneticDipolPower = 10;

	/**
	 * Maximum magnetic force.
	 */
	public double maxMagneticForce = 1000;

	/**
	 * Repeal between two vertexs.
	 */
	public double vertexElectricRepealCoefficient = 10000;

	/**
	 * Maximum electric repeal force.
	 */
	public double maxElectricForce = 1000;

	/**
	 * Friction reduces the energy of the system. The friction force points in the opposite direction of the current velocity.
	 * Friction is automatically updated during the solution. The current kinetic energy is compared against the average kinetic
	 * energy reduced by the time spent on the calculation and the friction coefficient is updated so that towards the end of the
	 * calculation the kinetic energy of the system converges to zero.
	 */
	public double frictionCoefficient = 1;
	
	/**
	 * This gets updated.
	 */
	private double updatedFrictionCoefficient;
	
	/**
	 * Lower limit for friction coefficient.
	 */
	public double minFrictionCoefficient = 0.1;
	
	/**
	 * Higher limit for friction coefficient update.
	 */
	public double maxFrictionCoefficient = 10;
	
	/**
	 * Multiplier used to update the friction coefficient.
	 */
	public double frictionCoefficientMultiplier = 2;

	/**
	 * The default time step when solution starts.
	 * Time step is automatically updated during the solution. It will always have the highest value so that the acceleration error
	 * is less than the acceleration error limit. The time step is either multiplied or divided by the time step multiplier according to
	 * the current acceleration error.
	 */
	public double timeStep = 1;
	
	/**
	 * This gets updated.
	 */
	private double updatedTimeStep;

	/**
	 * Lower limit for time step update.
	 */
	public double minTimeStep = 0.001;

	/**
	 * Lower limit for time step update.
	 */
	public double maxTimeStep = 1000;

	/**
	 * Multiplier used to update the time step.
	 */
	public double timeStepMultiplier = 2;

	/**
	 * Maximum time spend on the calculation in milliseconds.
	 * The algorithm will always return after this time has elapsed.
	 */
	public double calculationTimeLimit = 10000;

	/**
	 * Limit of acceleration approximation difference (between a1, a2, a3 and a4 in RK-4).
	 * Acceleration error limit is updated automatically during the solution. It decreases towards zero proportional to the
	 * time spent on the calculation.
	 */
	public double accelerationErrorLimit = 1;
	
	/**
	 * This gets updated.
	 */
	private double updatedAccelerationErrorLimit;

	/**
	 * Acceleration limit during the solution.
	 * When all vertices has lower acceleration than this limit then the algorithm may be stopped.
	 */
	public double accelerationRelaxLimit = 1;

	/**
	 * Velocity limit during the solution.
	 * When all vertices has lower velocity than this limit then the algorithm may be stopped.
	 */
	public double velocityRelaxLimit = 0.1;

	/**
	 * Maximim velocity;
	 */
	public double maxVelocity = 100;

	public ForceDirectedEmbedding(GraphComponent graphComponent) {
		this(graphComponent, null);
	}

	public ForceDirectedEmbedding(GraphComponent graphComponent, IForceDirectedEmbeddingListener listener) {
		super(graphComponent);
		this.listener = listener;
	}

	/**
	 * Find the solution for the differential equation using a
     * modified Runge-Kutta-4 algorithm.
     *
     * a1 = a[pn, vn]
     * a2 = a[pn + h / 2 * vn + h * h / 8 * a1, vn + h / 2 * a1]
     * a3 = a[pn + h / 2 * vn + h * h / 8 * a1, vn + h / 2 * a2]
     * a4 = a[pn + h * vn + h * h / 2 * a3, vn + h * a3]
     *
     * pn+1 = pn + h * vn + h * h / 6 * [a1 + a2 + a3]
     * vn+1 = vn + h / 6 * (a1 + 2 * a2 + 2 * a3 + a4)
     * 
     * This algorithm adaptively modifies timeStep and friction.
     */
	public void embed() {
		long begin = System.currentTimeMillis();
		int cycle = 0;
		int probCycle = 0;
		boolean relaxed = false;
		double timeStepSum = 0;
		double energySum = 0;
		double energy;
		// vn : positions
		Pt[] pn = createPtArray();
		// accelerations
		Pt[] an = createPtArray();
		// vn : velocities
		Pt[] vn = createPtArray();
		// as : accelerations
		Pt[] a1 = createPtArray();
		Pt[] a2 = createPtArray();
		Pt[] a3 = createPtArray();
		Pt[] a4 = createPtArray();
		// velocity deltas
		Pt[] dvn = createPtArray();
		Pt[] tvn = createPtArray();
		// position deltas
		Pt[] dpn = createPtArray();
		Pt[] tpn = createPtArray();
		// initialize random for no distance repealing
		// make it stable and always use the same seed
		random = new Random(0);

		// initial values
		getInitialPositions(pn);
		updatedTimeStep = timeStep;
		updatedAccelerationErrorLimit = accelerationErrorLimit;
		updatedFrictionCoefficient = frictionCoefficient;

		// Runge Kutta 4th order
		while (!relaxed) {
			long time;
			double hMultiplier = 0;
			double accelerationError = 0;
			cycle++;

			while (true) {
				probCycle++;

				// update acceleration error limit
				time = System.currentTimeMillis() - begin;
				if (time > calculationTimeLimit)
					break;
				else
					updatedAccelerationErrorLimit = (calculationTimeLimit - time) / calculationTimeLimit * this.accelerationErrorLimit;
				if (debug)
					System.out.println("Prob cycle at real time: " + time + " time: " + timeStepSum + " h: " + updatedTimeStep + " friction: " + updatedFrictionCoefficient + " acceleration error limit: " + updatedAccelerationErrorLimit + " acceleration error: " + accelerationError + " cycle: " + cycle + " prob cycle: " + probCycle);

				// a1 = a[pn, vn]
				a(a1, pn, vn);

				// a2 = a[pn + h / 2 * vn + h * h / 8 * a1, vn + h / 2 * a1]
				addMultiplied(tpn, pn, updatedTimeStep / 2, vn);
				incrementWithMultiplied(tpn, updatedTimeStep * updatedTimeStep / 8, a1);
				addMultiplied(tvn, vn, updatedTimeStep / 2, a1);
				a(a2, tpn, tvn);

				// a3 = a[pn + h / 2 * vn + h * h / 8 * a1, vn + h / 2 * a2]
				addMultiplied(tpn, pn, updatedTimeStep / 2, vn);
				incrementWithMultiplied(tpn, updatedTimeStep * updatedTimeStep / 8, a1);
				addMultiplied(tvn, vn, updatedTimeStep / 2, a2);
				a(a3, tpn, tvn);

				// a4 = a[pn + h * vn + h * h / 2 * a3, vn + h * a3]
				addMultiplied(tpn, pn, updatedTimeStep, vn);
				incrementWithMultiplied(tpn, updatedTimeStep * updatedTimeStep / 2, a3);
				addMultiplied(tvn, vn, updatedTimeStep, a3);
				a(a4, tpn, tvn);

				// Adjust time step (h)
				accelerationError = diff(a1, a2, a3, a4);
				
				if (accelerationError < updatedAccelerationErrorLimit) {
					if (hMultiplier == 0)
						hMultiplier = timeStepMultiplier;
					else
						break;
				}
				else {
					if (hMultiplier == 0)
						hMultiplier = timeStepMultiplier;
					else if (hMultiplier == timeStepMultiplier)
						hMultiplier = 1.0 / timeStepMultiplier;
				}
				
				double hNext = updatedTimeStep * hMultiplier;

				if (hNext < minTimeStep || hNext > maxTimeStep)
					break;
				
				updatedTimeStep *= hMultiplier;
			}

			// pn+1 = pn + h * vn + h * h / 6 * [a1 + a2 + a3]
			add(dpn, a1, a2);
			increment(dpn, a3);
			multiply(dpn, updatedTimeStep * updatedTimeStep / 6);
			incrementWithMultiplied(dpn, updatedTimeStep, vn);
			increment(pn, dpn);

			// vn+1 = vn + h / 6 * (a1 + 2 * a2 + 2 * a3 + a4)
			addMultiplied(dvn, a1, 2, a2);
			incrementWithMultiplied(dvn, 2, a3);
			increment(dvn, a4);
			multiply(dvn, updatedTimeStep / 6);
			increment(vn, dvn);
			// Maximize velocity
			for (int i = 0; i < vn.length; i++) {
				if (vn[i].getLength() > maxVelocity) {
					vn[i].normalize();
					vn[i].multiply(maxVelocity);
				}
			}

			// move vertices
			setVertexPositions(pn);

			// check time limit
			time = System.currentTimeMillis() - begin;
			if (time > calculationTimeLimit)
				break;
			if (debug)
				System.out.println("Cycle at real time: " + time + " time: " + timeStepSum + " h: " + updatedTimeStep + " friction: " + updatedFrictionCoefficient + " acceleration error limit: " + updatedAccelerationErrorLimit + " acceleration error: " + accelerationError + " cycle: " + cycle + " prob cycle: " + probCycle);

			// update friction
			timeStepSum += updatedTimeStep;
			energy = 0;
			for (int i = 0; i < vn.length; i++) {
				double velocity = vn[i].getLength();
				energy +=  1.0 / 2.0 * graphComponent.getVertex(i).mass * velocity * velocity;
			}
			energySum += energy * updatedTimeStep;
			double energyAvg = (energySum / timeStepSum);
			double energyExpected = energyAvg * (calculationTimeLimit - time) / calculationTimeLimit;
			if (debug)
				System.out.println("Energy: " + energy + " energy expected: " + energyExpected);
			if (energy <  energyExpected && updatedFrictionCoefficient / frictionCoefficientMultiplier > minFrictionCoefficient)
				updatedFrictionCoefficient /= frictionCoefficientMultiplier;
			else if (energy > energyExpected && updatedFrictionCoefficient * frictionCoefficientMultiplier < maxFrictionCoefficient)
				updatedFrictionCoefficient *= frictionCoefficientMultiplier;

			// Check if relaxed
			relaxed = true;
			for (int i = 0; i < vn.length; i++) {
				an[i].assign(a1[i]).add(a2[i]).add(a3[i]).add(a4[i]).divide(4);

				if (vn[i].getLength() > velocityRelaxLimit) {
					if (debug)
						System.out.println("Not relax due to velocity: " + vn[i].getLength());
					relaxed = false;
					break;
				}

				if (an[i].getLength() > accelerationRelaxLimit) {
					if (debug)
						System.out.println("Not relax due to acceleration: " + an[i].getLength());
					relaxed = false;
					break;
				}
			}
		}

		setFinalVertexPositions(pn);

		long end = System.currentTimeMillis();
		System.out.println("Runge-Kutta-4 ended, at real time: " + (end - begin) + " cycle: " + cycle + " prob cycle: " + probCycle);
	}

	private void getInitialPositions(Pt[] pn) {
		for (int i = 0; i < graphComponent.getVertexCount(); i++)
			pn[i] = graphComponent.getVertex(i).getCenter();
	}

	private Pt[] createPtArray() {
		Pt[] pts = new Pt[graphComponent.getVertexCount()];

		for (int i = 0; i < pts.length; i++)
			pts[i] = new Pt(0, 0);

		return pts;
	}

	// Calculation
	private double diff(Pt[] a1, Pt[] a2, Pt[] a3, Pt[] a4)
	{
		double max = 0;
		Assert.isTrue((a1.length == a2.length));
		Assert.isTrue(a2.length == a3.length);
		Assert.isTrue(a3.length == a4.length);

		for (int i = 0; i < a1.length; i++) {
			max = Math.max(max, a1[i].getDistance(a2[i]));
			max = Math.max(max, a2[i].getDistance(a3[i]));
			max = Math.max(max, a3[i].getDistance(a4[i]));
		}

		return max;
	}

	private void setVertexPositions(Pt[] pn)
	{
		Assert.isTrue(pn.length == graphComponent.getVertexCount());

		for (int i = 0; i < graphComponent.getVertexCount(); i++) {
			Vertex vertex = graphComponent.getVertex(i);

			if (vertex.anchor == null) {
				vertex.pt.x = pn[i].x - vertex.rs.width / 2;
				vertex.pt.y = pn[i].y - vertex.rs.height / 2;
			}
		}

		if (listener != null)
			listener.positionsChanged();
	}

	private void setFinalVertexPositions(Pt[] pn)
	{
		for (int i = 0; i < graphComponent.getVertexCount(); i++) {
			Vertex vertex = graphComponent.getVertex(i);
			
			if (vertex.positionConstraint != null)
				vertex.pt = vertex.positionConstraint.getFinalPosition(vertex.pt);
		}

		if (listener != null)
			listener.positionsChanged();
	}

	// pts = a
	private void assign(Pt[] pts, Pt[] a)
	{
		Assert.isTrue(a.length == a.length);
		Assert.isTrue(pts != a);

		for (int i = 0; i < pts.length; i++)
			pts[i].assign(a[i]);
	}

	// pts = a + b * c
	private void addMultiplied(Pt[] pts, Pt[] a, double b, Pt[] c)
	{
		Assert.isTrue(a.length == c.length);
		Assert.isTrue(pts.length == c.length);
		Assert.isTrue(pts != a);
		Assert.isTrue(pts != c);

		for (int i = 0; i < pts.length; i++)
			pts[i].assign(c[i]).multiply(b).add(a[i]);
	}

	// pts += a * b
	private void incrementWithMultiplied(Pt[] pts, double a, Pt[] b)
	{
		Assert.isTrue(pts.length == b.length);
		Assert.isTrue(pts != b);

		for (int i = 0; i < pts.length; i++) {
			Pt pt = pts[i];
			double x = pt.x;
			double y = pt.y;

			pts[i].assign(b[i]).multiply(a).add(x, y);
		}
	}

	// pts = a + b
	private void add(Pt[] pts, Pt[] a, Pt[] b)
	{
		Assert.isTrue(a.length == b.length);
		Assert.isTrue(pts != a);
		Assert.isTrue(pts != b);

		for (int i = 0; i < pts.length; i++)
			pts[i].assign(a[i]).add(b[i]);
	}

	// pts += a
	private void increment(Pt[] pts, Pt[] a)
	{
		Assert.isTrue(pts.length == a.length);
		Assert.isTrue(pts != a);

		for (int i = 0; i < pts.length; i++)
			pts[i].add(a[i]);
	}

	// pts *= a
	private void multiply(Pt[] pts, double a)
	{
		for (int i = 0; i < pts.length; i++)
			pts[i].multiply(a);
	}

	// an = a[pn, vn]
	private void a(Pt[] an, Pt[] pn, Pt[] vn)
	{
		Pt ptDistance = new Pt(0, 0);
		Rc rc1 = new Rc(0, 0, 0, 0);
		Rc rc2 = new Rc(0, 0, 0, 0);
		
		// Reset an
		for (int i = 0; i < an.length; i++) {
			an[i].setZero();
		}

		// Reset per vertex forces
		if (listener != null)
			for (Vertex vertex : graphComponent.getVertices())
				vertex.fs.clear();
		
		// Apply connection spring force
		if (connectionSpringCoefficient != 0)
			for (int i = 0; i < graphComponent.getEdgeCount(); i++) {
				Edge edge = graphComponent.getEdge(i);
				Vertex vertex1 = edge.source;
				Vertex vertex2 = edge.target;

				if (vertex1 != null && vertex2 != null && vertex1 != vertex2) {
					int i1 = graphComponent.IndexOfVertex(edge.source);
					int i2 = graphComponent.IndexOfVertex(edge.target);

					ptDistance.assign(pn[i2]).subtract(pn[i1]);
					ptDistance.setNaNToZero();
					double distance = ptDistance.getLength();

					if (distance != 0) {
						double springForce = connectionSpringCoefficient * edge.springCoefficient * distance;

						if (springForce > maxSpringForce)
							springForce = maxSpringForce;

						addForcePair(vertex1, vertex2, an[i1], an[i2], springForce, ptDistance, distance);
					}
				}
			}

		// Apply magnetic force
		if (connectionMagneticFieldCoefficient != 0)
			for (int i = 0; i < graphComponent.getEdgeCount(); i++) {
				Edge edge1 = graphComponent.getEdge(i);
				int iSource = graphComponent.IndexOfVertex(edge1.source);
				int iTarget = graphComponent.IndexOfVertex(edge1.target);
				Pt pSource1 = pn[iSource];
				Pt pTarget1 = pn[iTarget];
				Pt connectionCenter1 = new Pt(pSource1).add(pTarget1).divide(2);
				Pt magneticFieldPower = connectionHomogenousMagneticFieldPower;
				Pt m1 = new Pt(pSource1).subtract(pTarget1);
				double connectionLength = m1.getLength();

				if (connectionLength != 0) {
					m1.normalize();
					m1.multiply(connectionMagneticDipolPower);

					for (int j = i + 1; j < graphComponent.getEdgeCount(); j++) {
						Edge edge2 = graphComponent.getEdge(j);
						Pt pSource2 = pn[graphComponent.IndexOfVertex(edge2.source)];
						Pt pTarget2 = pn[graphComponent.IndexOfVertex(edge2.target)];
						Pt connectionCenter2 = new Pt(pSource2).add(pTarget2).divide(2);
						Pt m2 = new Pt(pTarget2).subtract(pSource2);
						m2.normalize();
						m2.multiply(connectionMagneticDipolPower);

						// magnetic dipol field power
						// B = (3(m*r)r - mr^2) / r^5
						Pt r = new Pt(connectionCenter1).subtract(connectionCenter2);
						double rLength = r.getLength();
						Pt dipolFieldPower = (r.copy().multiply(3 * m2.crossProduct(r)).subtract(m2.copy().multiply(Math.pow(rLength, 2)))).divide(Math.pow(rLength, 5)).multiply(connectionMagneticDipolPower);

						magneticFieldPower.add(dipolFieldPower);
					}

					double magneticFieldMoment = magneticFieldPower.x * m1.y - magneticFieldPower.y * m1.x;
					double magneticFieldForce = connectionMagneticFieldCoefficient * magneticFieldMoment / (connectionLength / 2);

					if (Math.abs(magneticFieldForce) > maxMagneticForce)
						magneticFieldForce = Math.signum(magneticFieldForce) * maxMagneticForce;
					else if (Double.isNaN(magneticFieldForce))
						magneticFieldForce = maxMagneticForce;

					an[iSource].x -= magneticFieldForce * m1.y / connectionLength;
					an[iSource].y += magneticFieldForce * m1.x / connectionLength;
					an[iTarget].x += magneticFieldForce * m1.y / connectionLength;
					an[iTarget].y -= magneticFieldForce * m1.x / connectionLength;
				}
			}

		// Apply electric force
		if (vertexElectricRepealCoefficient != 0)
			for (int i = 0; i < graphComponent.getVertexCount(); i++) {
				Vertex vertex1 = graphComponent.getVertex(i);
				rc1.assign(pn[i], vertex1.rs);

				for (int j = i + 1; j < graphComponent.getVertexCount(); j++) {
					Vertex vertex2 = graphComponent.getVertex(j);
					rc2.assign(pn[j], vertex2.rs);

// TODO:
//					if (!rc1.intersects(rc2)) {
						ptDistance.assign(pn[i]).subtract(pn[j]);
						if (!ptDistance.isNil()) {
							ptDistance.setNaNToZero();
							double distance = ptDistance.getLength();
							// TODO: find intersection with box
//							double modifiedDistance = distance - vertex1.rs.getDiagonalLength() / 2 - vertex2.rs.getDiagonalLength() / 2;
							double modifiedDistance = distance;
							double electricForce = vertexElectricRepealCoefficient * vertex1.charge * vertex2.charge / modifiedDistance / modifiedDistance;
	
							if (modifiedDistance <= 0 || electricForce > maxElectricForce)
								electricForce = maxElectricForce * vertex1.charge * vertex2.charge;
	
							addForcePair(vertex1, vertex2, an[i], an[j], electricForce, ptDistance, distance);
						}
//					}
				}
			}
		
		// Apply vertex spring force
		if (vertexSpringCoefficient != 0)
			for (int i = 0; i < graphComponent.getVertexCount(); i++) {
				Vertex vertex1 = graphComponent.getVertex(i);

				for (int j = i + 1; j < graphComponent.getVertexCount(); j++) {
					Vertex vertex2 = graphComponent.getVertex(j);
					ptDistance.assign(pn[j]).subtract(pn[i]);
					ptDistance.setNaNToZero();
					double distance = ptDistance.getLength();

					if (distance != 0) {
						double springForce = vertexSpringCoefficient * distance;

						if (springForce > maxSpringForce)
							springForce = maxSpringForce;

						addForcePair(vertex1, vertex2, an[i], an[j], springForce, ptDistance, distance);
					}
				}
			}

		// Apply vertex position constraint
		for (int i = 0; i < graphComponent.getVertexCount(); i++) {
			Vertex vertex = graphComponent.getVertex(i);

			if (vertex.positionConstraint != null) {
				Pt force = vertex.positionConstraint.getForce(pn[i]);
				addForce(vertex, an[i], force);
			}
		}

		// Apply friction force
		if (updatedFrictionCoefficient != 0)
			for (int i = 0; i <  graphComponent.getVertexCount(); i++) {
				double vlen = vn[i].getLength();
				// TODO: is it really quadratic?
				an[i].x -= updatedFrictionCoefficient * vlen * vn[i].x;
				an[i].y -= updatedFrictionCoefficient * vlen * vn[i].y;
			}

		// Convert forces to acceleration
		for (int i = 0; i < graphComponent.getVertexCount(); i++)
			an[i].divide(graphComponent.getVertex(i).mass);
	}

	private void addForcePair(Vertex vertex1, Vertex vertex2, Pt f1, Pt f2, double force, Pt d, double distance)
	{
		double forceX;
		double forceY;

		if (distance != 0) {
			forceX = force * d.x / distance;
			forceY = force * d.y / distance;
		}
		else {
			forceX = random.nextDouble() - 0.5;
			forceY = random.nextDouble() - 0.5;
		}

		Assert.isTrue(!Double.isNaN(forceX));
		Assert.isTrue(!Double.isNaN(forceY));

		if (f1 != null) {
			f1.x += forceX;
			f1.y += forceY;
		}

		if (f2 != null) {
			f2.x -= forceX;
			f2.y -= forceY;
		}

		if (listener != null && vertex1 != null)
			vertex1.fs.add(new Pt(forceX, forceY));

		if (listener != null && vertex2 != null)
			vertex2.fs.add(new Pt(-forceX, -forceY));
	}

	private void addForce(Vertex vertex, Pt f, Pt force) {
		if (f != null)
			f.add(force);

		if (vertex != null)
			vertex.fs.add(force);
	}

	public interface IForceDirectedEmbeddingListener {
		public void positionsChanged();
	}
}
