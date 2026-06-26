<template>
  <div class="page-container">
    <div class="page-header">
      <h2>统计报表</h2>
      <p>收入趋势、车辆状态与品牌分布</p>
    </div>

    <!-- Date Range Filter -->
    <div class="filter-bar">
      <el-date-picker
        v-model="dateRange"
        type="daterange"
        range-separator="至"
        start-placeholder="开始日期"
        end-placeholder="结束日期"
        value-format="YYYY-MM-DD"
        @change="loadRevenueData"
      />
      <el-select v-model="granularity" placeholder="时间粒度" style="width: 120px" @change="loadRevenueData">
        <el-option label="按日" value="daily" />
        <el-option label="按周" value="weekly" />
        <el-option label="按月" value="monthly" />
      </el-select>
    </div>

    <!-- Charts Grid -->
    <div class="charts-grid">
      <!-- Revenue Trend -->
      <el-card class="chart-card">
        <template #header>
          <span>收入趋势</span>
        </template>
        <v-chart :option="revenueOption" :autoresize="true" style="height: 320px" />
      </el-card>

      <!-- Vehicle Status Pie -->
      <el-card class="chart-card">
        <template #header>
          <span>车辆状态</span>
        </template>
        <v-chart :option="statusOption" :autoresize="true" style="height: 320px" />
      </el-card>
    </div>

    <div class="charts-grid" style="margin-top: 20px">
      <!-- Brand Distribution -->
      <el-card class="chart-card full-width">
        <template #header>
          <span>品牌分布</span>
        </template>
        <v-chart :option="brandOption" :autoresize="true" style="height: 320px" />
      </el-card>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart, PieChart, BarChart } from 'echarts/charts'
import {
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent,
} from 'echarts/components'
import VChart from 'vue-echarts'
import { getStatsRevenue, getStatsVehicles } from '@/api/finance'
import { ElMessage } from 'element-plus'

use([
  CanvasRenderer,
  LineChart, PieChart, BarChart,
  TitleComponent, TooltipComponent, LegendComponent,
  GridComponent,
])

const dateRange = ref([])
const granularity = ref('daily')

const revenueData = ref([])
const vehicleStats = ref({})

const revenueOption = computed(() => ({
  tooltip: { trigger: 'axis' },
  grid: { left: '3%', right: '4%', bottom: '3%', containLabel: true },
  xAxis: {
    type: 'category',
    data: revenueData.value.map(d => d.date),
    axisLabel: { rotate: 30 },
  },
  yAxis: { type: 'value', name: '收入 (¥)' },
  series: [{
    name: '收入',
    type: 'line',
    smooth: true,
    areaStyle: { opacity: 0.15 },
    data: revenueData.value.map(d => d.amount || 0),
    itemStyle: { color: '#c8a96e' },
  }],
}))

const statusOption = computed(() => {
  const stats = vehicleStats.value
  const data = [
    { name: '可用', value: stats.available || 0 },
    { name: '在租', value: stats.rented || 0 },
    { name: '维保', value: stats.maintenance || 0 },
  ]
  return {
    tooltip: { trigger: 'item' },
    legend: { bottom: '0%' },
    series: [{
      name: '车辆状态',
      type: 'pie',
      radius: ['40%', '70%'],
      avoidLabelOverlap: false,
      itemStyle: { borderRadius: 8, borderColor: '#111', borderWidth: 2 },
      label: { show: false, position: 'center' },
      emphasis: { label: { show: true, fontSize: 16, fontWeight: 'bold' } },
      labelLine: { show: false },
      data: data,
      color: ['#4ade80', '#60a5fa', '#fbbf24'],
    }],
  }
})

const brandOption = computed(() => {
  const data = vehicleStats.value.by_brand || []
  return {
    tooltip: { trigger: 'axis' },
    grid: { left: '3%', right: '4%', bottom: '3%', containLabel: true },
    xAxis: {
      type: 'category',
      data: data.map(d => d.brand),
    },
    yAxis: { type: 'value', name: '数量' },
    series: [{
      name: '数量',
      type: 'bar',
      data: data.map(d => d.count),
      itemStyle: {
        borderRadius: [4, 4, 0, 0],
        color: {
          type: 'linear', x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: '#c8a96e' },
            { offset: 1, color: '#a88b5a' },
          ],
        },
      },
      barMaxWidth: 48,
    }],
  }
})

async function loadRevenueData() {
  try {
    const params = { granularity: granularity.value }
    if (dateRange.value && dateRange.value.length === 2) {
      params.start_date = dateRange.value[0]
      params.end_date = dateRange.value[1]
    }
    const res = await getStatsRevenue(params)
    if (res.error === 0) {
      revenueData.value = res.items || []
    }
  } catch {
    ElMessage.error('加载收入数据失败')
  }
}

async function loadVehicleStats() {
  try {
    const res = await getStatsVehicles()
    if (res.error === 0) {
      vehicleStats.value = res
    }
  } catch {
    ElMessage.error('加载车辆统计失败')
  }
}

onMounted(() => {
  loadRevenueData()
  loadVehicleStats()
})
</script>

<style scoped>
.filter-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 24px;
}

.charts-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 20px;
}

.chart-card.full-width {
  grid-column: 1 / -1;
}
</style>