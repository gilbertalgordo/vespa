// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.tensor.functions;

import com.yahoo.tensor.Tensor;
import com.yahoo.tensor.TensorType;
import com.yahoo.tensor.evaluation.MapEvaluationContext;
import com.yahoo.tensor.evaluation.Name;
import com.yahoo.tensor.evaluation.TypeContext;
import com.yahoo.tensor.evaluation.VariableTensor;
import org.junit.Test;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.assertEquals;

/**
 * @author arnej
 */
public class EuclideanDistanceTestCase {

    @Test
    public void testVectorDistances() {
        var a = Tensor.from("tensor(x[3]):[1.0, 2.0, 3.0]");
        var b = Tensor.from("tensor(x[3]):[4.0, 2.0, 7.0]");
        var c = Tensor.from("tensor(x[3]):[1.0, 6.0, 6.0]");
        var op = new EuclideanDistance<>(new ConstantTensor<>(a), new ConstantTensor<>(b), "x");
        Tensor result = op.evaluate();
        assertEquals(5.0, result.asDouble(), 0.000001);
        op = new EuclideanDistance<>(new ConstantTensor<>(b), new ConstantTensor<>(a), "x");
        result = op.evaluate();
        assertEquals(5.0, result.asDouble(), 0.000001);
        op = new EuclideanDistance<>(new ConstantTensor<>(c), new ConstantTensor<>(a), "x");
        result = op.evaluate();
        assertEquals(5.0, result.asDouble(), 0.000001);
    }

    @Test
    public void testDistancesInMixed() {
        var a = Tensor.from("tensor(c{},x[3]):{foo:[1.0, 2.0, 3.0],bar:[0.0, 0.0, 0.0]}");
        var b = Tensor.from("tensor(c{},x[3]):{foo:[4.0, 2.0, 7.0],bar:[12.0, 0.0, 5.0]}");
        var op = new EuclideanDistance<>(new ConstantTensor<>(a), new ConstantTensor<>(b), "x");
        Tensor result = op.evaluate();
        var expect = Tensor.from("tensor(c{}):{foo:5.0,bar:13.0}");
        assertEquals(expect, result);
    }

    static class MyContext implements TypeContext<Name> {
        Map<String, TensorType> map = new HashMap<>();
        public TensorType getType(Name name) { return getType(name.name()); }
        public TensorType getType(String name) { return map.get(name); }
    }

    @Test
    public void testExpansion() {
        var tTypeA = TensorType.fromSpec("tensor(vecdim[128])");
        var tTypeB = TensorType.fromSpec("tensor(vecdim[128])");
        var a = new VariableTensor<>("left", tTypeA);
        var b = new VariableTensor<>("right", tTypeB);
        var op = new EuclideanDistance<>(a, b, "vecdim");
        assertEquals("map(reduce(map(join(left, right, f(a,b)(a - b)), f(a)(a * a)), sum, vecdim), f(a)(sqrt(a)))",
                     op.toPrimitive().toString());
        var context = new MyContext();
        context.map.put("left", tTypeA);
        context.map.put("right", tTypeB);
        var resType = op.type(context);
        assertEquals("tensor()", resType.toString());
    }

}
