<template>
  <div class="page-container">
    <div class="glass-card">
      <div class="glass-card-header">
        <h3>维保管理</h3>
        <div class="header-controls">
          <el-select v-model="query.status" placeholder="全部状态" clearable style="width: 120px" @change="loadList">
            <el-option label="待处理" value="pending" />
            <el-option label="进行中" value="in_progress" />
            <el-option label="已完成" value="completed" />
          </el-select>
          <el-select v-model="query.type" placeholder="全部类型" clearable style="width: 120px" @change="loadList">
            <el-option label="常规保养" value="regular" />
            <el-option label="维修" value="repair" />
            <el-option label="检测" value="inspection" />
            <el-option label="紧急维修" value="emergency" />
          </el-select>
          <el-button type="primary" @click="loadList">
            <el-icon><Search /></el-icon> 搜索
          </el-button>
          <el-button type="primary" @click="$router.push('/maintenance/add')">
            <el-icon><Plus /></el-icon> 添加记录
          </el-button>
        </div>
      </div>
      <div class="glass-card-body">
        <el-table :data="records" v-loading="loading" style="width: 100%" empty-text="暂无记录">
          <el-table-column prop="plate_number" label="车辆" min-width="120" />
          <el-table-column prop="type" label="类型" min-width="100">
            <template #default="{ row }">
              <el-tag effect="dark" size="small">{{ row.type }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="description" label="描述" min-width="180" show-overflow-tooltip />
          <el-table-column prop="cost" label="费用" min-width="100">
            <template #default="{ row }"><span class="text-accent">¥{{ row.cost || 0 }}</span></template>
          </el-table-column>
          <el-table-column prop="technician" label="技师" min-width="100" />
          <el-table-column prop="start_date" label="开始日期" min-width="110" />
          <el-table-column prop="status" label="状态" min-width="110">
            <template #default="{ row }">
              <el-tag :type="maintStatusType(row.status)" effect="dark" size="small">{{ row.status }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column label="操作" min-width="100">
            <template #default="{ row }">
              <el-button link type="primary" @click="$router.push(`/maintenance/${row.id}/edit`)">编辑</el-button>
              <el-button link type="danger" @click="handleDelete(row)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>

        <div class="pagination-wrapper" v-if="total > query.page_size">
          <el-pagination
            background
            layout="prev, pager, next"
            :total="total"
            :page-size="query.page_size"
            :current-page="query.page"
            @current-change="handlePageChange"
          />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { getMaintenanceList, deleteMaintenance } from '@/api/maintenance'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const loading = ref(false)
const records = ref([])
const total = ref(0)

const query = reactive({
  page: 1,
  page_size: 20,
  status: '',
  type: '',
})

function maintStatusType(status) {
  const map = {
    'pending': 'warning',
    'in_progress': 'primary',
    'completed': 'success',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

async function loadList() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.status) delete params.status
    if (!params.type) delete params.type
    const res = await getMaintenanceList(params)
    if (res.error === 0) {
      records.value = res.list || res.records || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载维保记录失败')
  } finally {
    loading.value = false
  }
}

function handlePageChange(page) {
  query.page = page
  loadList()
}

async function handleDelete(row) {
  try {
    await ElMessageBox.confirm('确定要删除这条维保记录吗？此操作不可撤销。', '删除确认', {
      confirmButtonText: '确定删除',
      cancelButtonText: '取消',
      type: 'warning',
    })
    const res = await deleteMaintenance({ id: row.id })
    if (res.error === 0) {
      ElMessage.success('维保记录已删除')
      loadList()
    } else {
      ElMessage.error(res.msg || '删除失败')
    }
  } catch (e) {
    if (e !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(loadList)
</script>

<style scoped>
.header-controls {
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  padding: 20px 0 8px;
}
</style>
