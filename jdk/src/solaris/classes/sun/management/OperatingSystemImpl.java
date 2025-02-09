/*
 * Copyright (c) 2003, 2019, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package sun.management;

import jdk.internal.platform.Metrics;
import java.util.concurrent.TimeUnit;

/**
 * Implementation class for the operating system.
 * Standard and committed hotspot-specific metrics if any.
 *
 * ManagementFactory.getOperatingSystemMXBean() returns an instance
 * of this class.
 */
class OperatingSystemImpl extends BaseOperatingSystemImpl
    implements com.sun.management.UnixOperatingSystemMXBean {

    private static final int MAX_ATTEMPTS_NUMBER = 10;
    private final Metrics containerMetrics;
    private long usageTicks = 0; // used for cpu load calculation
    private long totalTicks = 0; // used for cpu load calculation

    OperatingSystemImpl(VMManagement vm) {
        super(vm);
        this.containerMetrics = jdk.internal.platform.Container.metrics();
    }

    public long getTotalSwapSpaceSize() {
        if (containerMetrics != null) {
            long limit = containerMetrics.getMemoryAndSwapLimit();
            // The memory limit metrics is not available if JVM runs on Linux host (not in a docker container)
            // or if a docker container was started without specifying a memory limit (without '--memory='
            // Docker option). In latter case there is no limit on how much memory the container can use and
            // it can use as much memory as the host's OS allows.
            long memLimit = containerMetrics.getMemoryLimit();
            if (limit >= 0 && memLimit >= 0) {
                return limit - memLimit;
            }
        }
        return getTotalSwapSpaceSize0();
    }

    public long getFreeSwapSpaceSize() {
        if (containerMetrics != null) {
            long memSwapLimit = containerMetrics.getMemoryAndSwapLimit();
            long memLimit = containerMetrics.getMemoryLimit();
            if (memSwapLimit >= 0 && memLimit >= 0) {
                long deltaLimit = memSwapLimit - memLimit;
                // Return 0 when memSwapLimit == memLimit, which means no swap space is allowed.
                // And the same for memSwapLimit < memLimit.
                if (deltaLimit <= 0) {
                    return 0;
                }
                for (int attempt = 0; attempt < MAX_ATTEMPTS_NUMBER; attempt++) {
                    long memSwapUsage = containerMetrics.getMemoryAndSwapUsage();
                    long memUsage = containerMetrics.getMemoryUsage();
                    if (memSwapUsage > 0 && memUsage > 0) {
                        // We read "memory usage" and "memory and swap usage" not atomically,
                        // and it's possible to get the negative value when subtracting these two.
                        // If this happens just retry the loop for a few iterations.
                        long deltaUsage = memSwapUsage - memUsage;
                        if (deltaUsage >= 0) {
                            long freeSwap = deltaLimit - deltaUsage;
                            if (freeSwap >= 0) {
                                return freeSwap;
                            }
                        }
                    }
                }
            }
        }
        return getFreeSwapSpaceSize0();
    }

    public long getFreePhysicalMemorySize() {
        if (containerMetrics != null) {
            long usage = containerMetrics.getMemoryUsage();
            long limit = containerMetrics.getMemoryLimit();
            if (usage > 0 && limit >= 0) {
                return limit - usage;
            }
        }
        return getFreePhysicalMemorySize0();
    }

    public long getTotalPhysicalMemorySize() {
        if (containerMetrics != null) {
            long limit = containerMetrics.getMemoryLimit();
            if (limit >= 0) {
                return limit;
            }
        }
        return getTotalPhysicalMemorySize0();
    }

    private double getUsageDividesTotal(long usageTicks, long totalTicks) {
        // If cpu quota or cpu shares are in effect calculate the cpu load
        // based on the following formula (similar to how
        // getCpuLoad0() is being calculated):
        //
        //   | usageTicks - usageTicks' |
        //  ------------------------------
        //   | totalTicks - totalTicks' |
        //
        // where usageTicks' and totalTicks' are historical values
        // retrieved via an earlier call of this method.
        //
        // Total ticks should be scaled to the container effective number
        // of cpus, if cpu shares are in effect.
        if (usageTicks < 0 || totalTicks <= 0) {
            return -1;
        }
        long distance = usageTicks - this.usageTicks;
        this.usageTicks = usageTicks;
        long totalDistance = totalTicks - this.totalTicks;
        this.totalTicks = totalTicks;

        double systemLoad = 0.0;
        if (distance > 0 && totalDistance > 0) {
            systemLoad = ((double)distance) / totalDistance;
        }
        // Ensure the return value is in the range 0.0 -> 1.0
        systemLoad = Math.max(0.0, systemLoad);
        systemLoad = Math.min(1.0, systemLoad);
        return systemLoad;
    }

    public double getSystemCpuLoad() {
        if (containerMetrics != null) {
            long quota = containerMetrics.getCpuQuota();
            long share = containerMetrics.getCpuShares();
            long usageNanos = containerMetrics.getCpuUsage();
            if (quota > 0) {
                long numPeriods = containerMetrics.getCpuNumPeriods();
                long quotaNanos = TimeUnit.MICROSECONDS.toNanos(quota * numPeriods);
                return getUsageDividesTotal(usageNanos, quotaNanos);
            } else if (share > 0) {
                long hostTicks = getHostTotalCpuTicks0();
                int totalCPUs = getHostOnlineCpuCount0();
                int containerCPUs = getAvailableProcessors();
                // scale the total host load to the actual container cpus
                hostTicks = hostTicks * containerCPUs / totalCPUs;
                return getUsageDividesTotal(usageNanos, hostTicks);
            } else {
                // If CPU quotas and shares are not active then find the average system load for
                // all online CPUs that are allowed to run this container.

                // If the cpuset is the same as the host's one there is no need to iterate over each CPU
                if (isCpuSetSameAsHostCpuSet()) {
                    return getSystemCpuLoad0();
                } else {
                    int[] cpuSet = containerMetrics.getEffectiveCpuSetCpus();
                    // in case the effectiveCPUSetCpus are not available, attempt to use just cpusets.cpus
                    if (cpuSet == null || cpuSet.length <= 0) {
                        cpuSet = containerMetrics.getCpuSetCpus();
                    }
                    if (cpuSet != null && cpuSet.length > 0) {
                        double systemLoad = 0.0;
                        for (int cpu : cpuSet) {
                            double cpuLoad = getSingleCpuLoad0(cpu);
                            if (cpuLoad < 0) {
                                return -1;
                            }
                            systemLoad += cpuLoad;
                        }
                        return systemLoad / cpuSet.length;
                    }
                    return -1;
                }
            }
        }
        return getSystemCpuLoad0();
    }

    private boolean isCpuSetSameAsHostCpuSet() {
        if (containerMetrics != null) {
            return containerMetrics.getCpuSetCpus().length == getHostOnlineCpuCount0();
        }
        return false;
    }

    public native long getCommittedVirtualMemorySize();
    private native long getTotalSwapSpaceSize0();
    private native long getFreeSwapSpaceSize0();
    public native long getProcessCpuTime();
    private native long getFreePhysicalMemorySize0();
    private native long getTotalPhysicalMemorySize0();
    public native long getOpenFileDescriptorCount();
    public native long getMaxFileDescriptorCount();
    private native double getSystemCpuLoad0();
    public native double getProcessCpuLoad();
    private native double getSingleCpuLoad0(int cpuNum);
    private native int getHostConfiguredCpuCount0();
    private native int getHostOnlineCpuCount0();
    // CPU ticks since boot in nanoseconds
    private native long getHostTotalCpuTicks0();

    static {
        initialize();
    }
    private static native void initialize();
}
