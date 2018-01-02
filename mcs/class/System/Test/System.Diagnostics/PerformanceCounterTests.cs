// originally copied from external/corefx

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections;
using System.Collections.Specialized;
using NUnit.Framework;	
using System.Diagnostics;

namespace MonoTests.System.Diagnostics.TestPerformanceCounter
{
	internal class Common
	{
		internal string name;
		internal string category;
		internal CounterCreationDataCollection counterDataCollection;
		internal CounterCreationData counter;
		internal CounterCreationData counterBase;
		internal PerformanceCounter counterSample;
	
		internal void DoCommon()
		{
			name = Guid.NewGuid().ToString("N") + "_Counter";
			category = name + "_Category";
			counterDataCollection = new CounterCreationDataCollection();

			// Add the counter.
			counter = new CounterCreationData();
			counter.CounterType = PerformanceCounterType.AverageCount64;
			counter.CounterName = name;
			counterDataCollection.Add(counter);

			// Add the base counter.
			counterBase = new CounterCreationData();
			counterBase.CounterType = PerformanceCounterType.AverageBase;
			counterBase.CounterName = name + "Base";
			counterDataCollection.Add(counterBase);

			// Create the category.
			PerformanceCounterCategory.Create(category,
			    "description",
			    PerformanceCounterCategoryType.SingleInstance, counterDataCollection);

			counterSample = new PerformanceCounter(category, 
				name, false);

			counterSample.RawValue = 0;
		}
	}

    [TestFixture]
    public static class PerformanceCounterTests
    {
        [Test]
        public static void PerformanceCounterCategory_CreateCategory()
        {
            if ( !PerformanceCounterCategory.Exists("AverageCounter64SampleCategory") ) 
            {
                CounterCreationDataCollection counterDataCollection = new CounterCreationDataCollection();

                // Add the counter.
                CounterCreationData averageCount64 = new CounterCreationData();
                averageCount64.CounterType = PerformanceCounterType.AverageCount64;
                averageCount64.CounterName = "AverageCounter64Sample";
                counterDataCollection.Add(averageCount64);

                // Add the base counter.
                CounterCreationData averageCount64Base = new CounterCreationData();
                averageCount64Base.CounterType = PerformanceCounterType.AverageBase;
                averageCount64Base.CounterName = "AverageCounter64SampleBase";
                counterDataCollection.Add(averageCount64Base);

                // Create the category.
                PerformanceCounterCategory.Create("AverageCounter64SampleCategory",
                    "Demonstrates usage of the AverageCounter64 performance counter type.",
                    PerformanceCounterCategoryType.SingleInstance, counterDataCollection);
            }

            Assert.True(PerformanceCounterCategory.Exists("AverageCounter64SampleCategory"));
        }

        [Test]
        public static void PerformanceCounter_CreateCounter_Count0()
        {
        	var a = new Common();
        	a.DoCommon();
		Assert.AreEqual(0, a.counterSample.RawValue);
        	a.counterSample.Increment();
		Assert.AreEqual(1, a.counterSample.RawValue);
        }
        
        [Test]
        public static void PerformanceCounter_InstanceNames()
        {
        	var a = new Common();
        	a.DoCommon();
		var pcc = new PerformanceCounterCategory(a.category);
		var names = pcc.GetInstanceNames();
		int i = 0;
		Console.WriteLine("PerformanceCounter_InstanceNames {0} {1} {2} {3}", a.name, pcc, names, names.Length);
		foreach (var b in names)
			Console.WriteLine("{0}:{1}", i++, b);
		Assert.AreEqual(names.Length, 1);
        }

        [Test]
        public static void PerformanceCounter_Counters()
        {
        	var a = new Common();
        	a.DoCommon();
		var pcc = new PerformanceCounterCategory(a.category);
		var counters = pcc.GetCounters(a.name);
		int i = 0;
		// In case of failure, dump all the information before any asserts.
		Console.WriteLine("PerformanceCounter_Counters {0} {1} {2}", a.name, counters, counters.Length);
		foreach (var b in counters)
			Console.WriteLine("i:{0} counter:{1} cat:{2} cname:{3} iname:{4} val:{5}", i++, b, b.CategoryName, b.CounterName, b.InstanceName, b.RawValue);
		Assert.AreEqual(counters.Length, 2);
        }
    }
}
